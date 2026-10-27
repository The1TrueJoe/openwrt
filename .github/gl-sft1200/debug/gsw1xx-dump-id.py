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
