# Debug build only: make mxl-gsw1xx print the raw chip-ID registers and a
# few register blocks, and carry on when the chip-ID check fails.
# Usage: python3 gsw1xx-dump-id.py <path to mxl-gsw1xx.c>
import sys

path = sys.argv[1]
src = open(path).read()

anchor = (
    "\tret = regmap_read(priv->shell, GSW1XX_SHELL_MANU_ID, &val);\n"
    "\tif (ret < 0)\n"
    "\t\treturn ret;\n"
)
assert src.count(anchor) == 1, "MANU_ID read not found"
dump = anchor + r'''
	/* DEBUG: dump the ID registers and some register blocks */
	{
		static const struct { unsigned int base, n; } blk[] = {
			{ GSW1XX_SHELL_BASE, 0x20 }, { GSW1XX_SWITCH_BASE, 0x20 },
			{ GSW1XX_RGMII_BASE, 0x10 }, { GSW1XX_GPIO_BASE, 0x10 },
			{ GSW1XX_MMDIO_BASE, 0x10 }, { GSW1XX_CLK_BASE, 0x10 },
		};
		struct mii_bus *bus = mdiodev->bus;
		unsigned int b, r, v[8];
		int i;

		dev_info(dev, "DEBUG MANU_ID (0x%x) = 0x%04x\n",
			 GSW1XX_SHELL_BASE + GSW1XX_SHELL_MANU_ID, val);
		for (b = 0; b < ARRAY_SIZE(blk); b++) {
			for (r = 0; r < blk[b].n; r += 8) {
				mutex_lock(&bus->mdio_lock);
				for (i = 0; i < 8; i++)
					if (gsw1xx_regmap_read(priv, blk[b].base + r + i, &v[i]))
						v[i] = 0xdead;
				mutex_unlock(&bus->mdio_lock);
				dev_info(dev, "DEBUG 0x%04x: %04x %04x %04x %04x %04x %04x %04x %04x\n",
					 blk[b].base + r, v[0], v[1], v[2], v[3],
					 v[4], v[5], v[6], v[7]);
			}
		}
		/* raw SMDIO registers at this address, BADR left as is */
		mutex_lock(&bus->mdio_lock);
		for (i = 0; i < 32; i++)
			dev_info(dev, "DEBUG smdio reg 0x%02x = 0x%04x\n", i,
				 __mdiobus_read(bus, mdiodev->addr, i) & 0xffff);
		mutex_unlock(&bus->mdio_lock);
		priv->smdio_badr = GSW1XX_SMDIO_BADR_UNKNOWN;
	}
'''
src = src.replace(anchor, dump)

checks = [
    "\tif (FIELD_GET(GSW1XX_SHELL_MANU_ID_FIX1, val) != 1)\n\t\treturn -ENODEV;\n",
    "\tif (FIELD_GET(GSW1XX_SHELL_MANU_ID_MANID, val) !=\n\t    GSW1XX_SHELL_MANU_ID_MANID_VAL)\n\t\treturn -ENODEV;\n",
]
for c in checks:
    assert src.count(c) == 1, "check not found: " + c
    src = src.replace(c, c.replace("\t\treturn -ENODEV;\n",
        '\t\tdev_warn(dev, "DEBUG: chip ID check failed at line %d, continuing\\n", __LINE__);\n'))

pn = "\tshellver = FIELD_GET(GSW1XX_SHELL_PNUM_ID_VER, val);\n"
assert src.count(pn) == 1
src = src.replace(pn, pn + '\tdev_info(dev, "DEBUG PNUM_ID (0x%x) = 0x%04x\\n",\n\t\t GSW1XX_SHELL_BASE + GSW1XX_SHELL_PNUM_ID, val);\n')

open(path, "w").write(src)
print("patched", path)
