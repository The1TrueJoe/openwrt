// SPDX-License-Identifier: GPL-2.0-only

#include <linux/array_size.h>
#include <linux/clk.h>
#include <linux/mod_devicetable.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/reset.h>

#define USB_SLEEPM 0x4

struct sf19a2890_usb_phy {
	struct device *dev;
	struct clk *phy_clk;
	/* Asserted in this order, deasserted in reverse */
	struct reset_control_bulk_data resets[2];
	void __iomem *base;
};

static int sf19a2890_usb_phy_power_on(struct phy *phy)
{
	struct sf19a2890_usb_phy *p_phy = phy_get_drvdata(phy);
	int ret;

	ret = clk_prepare_enable(p_phy->phy_clk);
	if (ret) {
		dev_err(p_phy->dev, "failed to enable the PHY clock: %d\n",
			ret);
		return ret;
	}

	ret = reset_control_bulk_deassert(ARRAY_SIZE(p_phy->resets),
					  p_phy->resets);
	if (ret)
		goto err_disable_clk;

	writel(1, p_phy->base + USB_SLEEPM);
	return 0;

err_disable_clk:
	clk_disable_unprepare(p_phy->phy_clk);
	return ret;
}

static int sf19a2890_usb_phy_power_off(struct phy *phy)
{
	struct sf19a2890_usb_phy *p_phy = phy_get_drvdata(phy);

	writel(0, p_phy->base + USB_SLEEPM);
	reset_control_bulk_assert(ARRAY_SIZE(p_phy->resets), p_phy->resets);
	clk_disable_unprepare(p_phy->phy_clk);
	return 0;
}

static const struct phy_ops sf19a2890_usb_phy_ops = {
	.power_on = sf19a2890_usb_phy_power_on,
	.power_off = sf19a2890_usb_phy_power_off,
	.owner = THIS_MODULE,
};

static int sf19a2890_usb_phy_probe(struct platform_device *pdev)
{
	struct sf19a2890_usb_phy *p_phy;
	struct phy_provider *provider;
	struct phy *phy;
	int ret;

	p_phy = devm_kzalloc(&pdev->dev, sizeof(*p_phy), GFP_KERNEL);
	if (!p_phy)
		return -ENOMEM;

	p_phy->dev = &pdev->dev;

	p_phy->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(p_phy->base))
		return PTR_ERR(p_phy->base);

	p_phy->phy_clk = devm_clk_get(p_phy->dev, NULL);
	if (IS_ERR(p_phy->phy_clk))
		return dev_err_probe(p_phy->dev, PTR_ERR(p_phy->phy_clk),
				     "failed to get the PHY clock\n");

	p_phy->resets[0].id = "por";
	p_phy->resets[1].id = "phy";
	ret = devm_reset_control_bulk_get_exclusive(p_phy->dev,
						    ARRAY_SIZE(p_phy->resets),
						    p_phy->resets);
	if (ret)
		return dev_err_probe(p_phy->dev, ret,
				     "failed to get the resets\n");

	ret = reset_control_bulk_assert(ARRAY_SIZE(p_phy->resets),
					p_phy->resets);
	if (ret)
		return ret;

	phy = devm_phy_create(p_phy->dev, NULL, &sf19a2890_usb_phy_ops);
	if (IS_ERR(phy))
		return dev_err_probe(p_phy->dev, PTR_ERR(phy),
				     "failed to create the PHY\n");

	phy_set_drvdata(phy, p_phy);

	provider = devm_of_phy_provider_register(p_phy->dev,
						 of_phy_simple_xlate);
	if (IS_ERR(provider))
		return dev_err_probe(p_phy->dev, PTR_ERR(provider),
				     "failed to register the PHY provider\n");

	return 0;
}

static const struct of_device_id sf19a2890_usb_phy_of_match[] = {
	{ .compatible = "siflower,sf19a2890-usb-phy" },
	{ }
};
MODULE_DEVICE_TABLE(of, sf19a2890_usb_phy_of_match);

static struct platform_driver sf19a2890_usb_phy_driver = {
	.probe		= sf19a2890_usb_phy_probe,
	.driver		= {
		.name	= "sf19a2890-usb-phy",
		.of_match_table = sf19a2890_usb_phy_of_match,
	},
};
module_platform_driver(sf19a2890_usb_phy_driver);

MODULE_AUTHOR("Ziying Wu <ziying.wu@siflower.com.cn>");
MODULE_DESCRIPTION("Siflower SF19A2890 USB 2.0 PHY driver");
MODULE_LICENSE("GPL");
