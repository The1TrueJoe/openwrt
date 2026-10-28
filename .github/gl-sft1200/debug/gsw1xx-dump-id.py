# Debug build only: let the mxl-gsw1xx setup hook's internal-PHY reset write
# (RST_REQ, from 82e2c7db0830) be overridden by module parameters, and log
# the shell registers and RST_REQ before and after it.
# Usage: python3 gsw1xx-dump-id.py <path to mxl-gsw1xx.c>
import sys

path = sys.argv[1]
src = open(path).read()

head = "static int gsw1xx_setup(struct dsa_switch *ds)\n{\n"
assert src.count(head) == 1, "gsw1xx_setup not found"
src = src.replace(head, r'''static int rst_override = -1;
module_param(rst_override, int, 0444);
MODULE_PARM_DESC(rst_override,
		 "DEBUG: -1 upstream, -2 skip the RST_REQ write, >= 0 value for the PHY reset bits");

static int rst_delay_ms = 300;
module_param(rst_delay_ms, int, 0444);
MODULE_PARM_DESC(rst_delay_ms, "DEBUG: settle delay after the RST_REQ write");

''' + head)

old = (
    "\tret = regmap_update_bits(priv->shell, GSW1XX_SHELL_RST_REQ, phy_mask, ~active_mask);\n"
    "\tif (ret)\n"
    "\t\treturn ret;\n"
    "\n"
    "\tif (active_mask)\n"
    "\t\tmsleep(300);\n"
)
assert src.count(old) == 1, "RST_REQ write not found"
src = src.replace(old, r'''	{
		unsigned int r, v[16], before = 0, after = 0;

		for (r = 0; r < 16; r++)
			if (regmap_read(priv->shell, r, &v[r]))
				v[r] = 0xdead;
		dev_info(ds->dev, "DEBUG shell 0xfa00: %04x %04x %04x %04x %04x %04x %04x %04x\n",
			 v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
		dev_info(ds->dev, "DEBUG shell 0xfa08: %04x %04x %04x %04x %04x %04x %04x %04x\n",
			 v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15]);

		regmap_read(priv->shell, GSW1XX_SHELL_RST_REQ, &before);
		dev_info(ds->dev, "DEBUG RST_REQ before 0x%04x, phy_mask 0x%x, active_mask 0x%x, rst_override %d\n",
			 before, phy_mask, active_mask, rst_override);
		if (rst_override == -2)
			ret = 0;
		else if (rst_override >= 0)
			ret = regmap_update_bits(priv->shell, GSW1XX_SHELL_RST_REQ,
						 phy_mask, rst_override);
		else
			ret = regmap_update_bits(priv->shell, GSW1XX_SHELL_RST_REQ,
						 phy_mask, ~active_mask);
		if (ret)
			return ret;

		msleep(rst_delay_ms);
		regmap_read(priv->shell, GSW1XX_SHELL_RST_REQ, &after);
		dev_info(ds->dev, "DEBUG RST_REQ after 0x%04x, waited %d ms\n",
			 after, rst_delay_ms);
	}
''')

open(path, "w").write(src)
print("patched", path)

# Second experiment: the external-MDIO pin mux and a scan of the internal
# MDIO bus once DSA is set up.
src = open(path).read()

inc = "#include <linux/module.h>\n"
assert src.count(inc) == 1, "module.h include not found"
src = src.replace(inc, inc + "#include <linux/of_mdio.h>\n")

alt = (
    "\tregmap_set_bits(priv->gpio, GPIO_ALTSEL0, 3);\n"
    "\tregmap_set_bits(priv->gpio, GPIO_ALTSEL1, 3);\n"
)
assert src.count(alt) == 1, "ALTSEL writes not found"
src = src.replace(alt, r'''	{
		unsigned int a0 = 0, a1 = 0;

		regmap_read(priv->gpio, GPIO_ALTSEL0, &a0);
		regmap_read(priv->gpio, GPIO_ALTSEL1, &a1);
		dev_info(dev, "DEBUG GPIO_ALTSEL0/1 before 0x%04x 0x%04x, skip_altsel %d\n",
			 a0, a1, skip_altsel);
		if (!skip_altsel) {
			regmap_set_bits(priv->gpio, GPIO_ALTSEL0, 3);
			regmap_set_bits(priv->gpio, GPIO_ALTSEL1, 3);
		}
		regmap_read(priv->gpio, GPIO_ALTSEL0, &a0);
		regmap_read(priv->gpio, GPIO_ALTSEL1, &a1);
		dev_info(dev, "DEBUG GPIO_ALTSEL0/1 after 0x%04x 0x%04x\n", a0, a1);
	}
''')

probe = "static int gsw1xx_probe(struct mdio_device *mdiodev)\n{\n"
assert src.count(probe) == 1
src = src.replace(probe, r'''static bool skip_altsel;
module_param(skip_altsel, bool, 0444);
MODULE_PARM_DESC(skip_altsel, "DEBUG: leave GPIO_ALTSEL0/1 as found");

''' + probe)

info = '\tdev_info(dev, "standalone switch part number 0x%x v1.%u\\n", pnum, shellver);\n'
assert src.count(info) == 1
src = src.replace(info, info + r'''
	{
		struct device_node *np = of_get_child_by_name(dev->of_node, "mdio");
		struct mii_bus *bus = np ? of_mdio_find_bus(np) : NULL;
		unsigned int r, v;
		int a;

		of_node_put(np);
		for (r = 0x0b; r <= 0x15; r++) {
			if (regmap_read(priv->gswip.mdio, r, &v))
				v = 0xdead;
			dev_info(dev, "DEBUG MMDIO 0x%04x = 0x%04x\n", GSW1XX_MMDIO_BASE + r, v);
		}
		if (bus) {
			for (a = 0; a < PHY_MAX_ADDR; a++)
				dev_info(dev, "DEBUG internal MDIO addr %2d: id %04x %04x bmcr %04x\n", a,
					 mdiobus_read(bus, a, MII_PHYSID1) & 0xffff,
					 mdiobus_read(bus, a, MII_PHYSID2) & 0xffff,
					 mdiobus_read(bus, a, MII_BMCR) & 0xffff);
			put_device(&bus->dev);
		} else {
			dev_info(dev, "DEBUG internal MDIO bus not found\n");
		}
	}
''')

open(path, "w").write(src)
print("patched (second experiment)", path)

# Third experiment: a vendor-style hardware reset at probe time.
src = open(path).read()
probe = "static int gsw1xx_probe(struct mdio_device *mdiodev)\n{\n"
assert src.count(probe) == 1
src = src.replace(probe, r'''static bool hw_reset;
module_param(hw_reset, bool, 0444);
MODULE_PARM_DESC(hw_reset, "DEBUG: pulse reset-gpios for 2 ms at probe, as the vendor driver does");

static int probe_delay_ms;
module_param(probe_delay_ms, int, 0444);
MODULE_PARM_DESC(probe_delay_ms, "DEBUG: wait this long at probe start, after any hw_reset");

''' + probe)
alloc = "\tpriv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);\n"
assert src.count(alloc) == 1
src = src.replace(alloc, r'''	dev_info(dev, "DEBUG probe: reset_gpio %s, reset_state %d, hw_reset %d, probe_delay_ms %d\n",
		 mdiodev->reset_gpio ? "present" : "none", mdiodev->reset_state,
		 hw_reset, probe_delay_ms);
	if (hw_reset) {
		mdio_device_reset(mdiodev, 1);
		usleep_range(2000, 3000);
		mdio_device_reset(mdiodev, 0);
	}
	if (probe_delay_ms)
		msleep(probe_delay_ms);

''' + alloc, 1)
open(path, "w").write(src)
print("patched (third experiment)", path)
