#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>


struct boe_panel {
	struct drm_panel base;
	struct mipi_dsi_device *dsi;

	struct gpio_desc *io_vdd_en_gpio;
	struct gpio_desc *vss_pn_en_gpio;
	struct gpio_desc *backlight_en_gpio;
	struct gpio_desc *n_reset_gpio;

	bool prepared;
	bool enabled;

	const struct drm_display_mode *mode;
};

static inline struct boe_panel *to_boe_panel(struct drm_panel *panel)
{
	return container_of(panel, struct boe_panel, base);
}

static int boe_panel_init(struct boe_panel *boe)
{
	struct mipi_dsi_device *dsi = boe->dsi;
	struct device *dev = &boe->dsi->dev;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	// -------------------- FIRST --------------------
	ret = mipi_dsi_dcs_write(dsi, 0xFF, (u8[]){ 0xE0 }, 1);
	if (ret < 0) {
		dev_err(dev, "failed to write 0xFF in FIRST stage: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write(dsi, 0xFB, (u8[]){ 0x01 }, 1);
	if (ret < 0) {
		dev_err(dev, "failed to write 0xFB in FIRST stage: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write(dsi, 0x53, (u8[]){ 0x22 }, 1);
	if (ret < 0) {
		dev_err(dev, "failed to write 0x53 in FIRST stage: %d\n", ret);
		return ret;
	}

	// -------------------- SECOND --------------------
	ret = mipi_dsi_dcs_write(dsi, 0xFF, (u8[]){ 0x25 }, 1);
	if (ret < 0) {
		dev_err(dev, "failed to write 0xFF in SECOND stage: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write(dsi, 0xFB, (u8[]){ 0x01 }, 1);
	if (ret < 0) {
		dev_err(dev, "failed to write 0xFB in SECOND stage: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write(dsi, 0xC4, (u8[]){ 0x10 }, 1);
	if (ret < 0) {
		dev_err(dev, "failed to write 0xC4 in SECOND stage: %d\n", ret);
		return ret;
	}

	// -------------------- THIRD --------------------
	ret = mipi_dsi_dcs_write(dsi, 0xFF, (u8[]){ 0x10 }, 1);
	if (ret < 0) {
		dev_err(dev, "failed to write 0xFF in THIRD stage: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write(dsi, 0xFB, (u8[]){ 0x01 }, 1);
	if (ret < 0) {
		dev_err(dev, "failed to write 0xFB in THIRD stage: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write(dsi, 0xC0, (u8[]){ 0x00 }, 1);
	if (ret < 0) {
		dev_err(dev, "failed to write 0xC0 in THIRD stage: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write(dsi, 0x3B, (u8[]){ 0x80, 0x0A, 0x00, 0x0A }, 4);
	if (ret < 0) {
		dev_err(dev, "failed to write 0x3B in THIRD stage: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write(dsi, 0xBE, (u8[]){ 0x00, 0x0A, 0x00, 0x0A }, 4);
	if (ret < 0) {
		dev_err(dev, "failed to write 0xBE in THIRD stage: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write(dsi, 0xBB, (u8[]){ 0x03 }, 1);
	if (ret < 0) {
		dev_err(dev, "failed to write 0xBB in THIRD stage %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write(dsi, 0xBA, (u8[]){ 0x07 }, 1);
	if (ret < 0) {
		dev_err(dev, "failed to write 0xBA in THIRD stage: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write(dsi, 0x35, (u8[]){ 0x00 }, 1);
	if (ret < 0) {
		dev_err(dev, "failed to write 0x35 in THIRD stage: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write(dsi, 0x36, (u8[]){ 0x00 }, 1);
	if (ret < 0) {
		dev_err(dev, "failed to write 0x36 in THIRD stage: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write(dsi, 0x2B, (u8[]){ 0x00, 0x00, 0x06, 0x40 }, 4);
	if (ret < 0) {
		dev_err(dev, "failed to write 0x2B in THIRD stage: %d\n", ret);
		return ret;
	}

	// -------------------- FOURTH --------------------
	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "failed to set exit sleep mode: %d\n", ret);
		return ret;
	}

	msleep(120);
	return 0;
}

static int boe_panel_on(struct boe_panel *boe)
{
	struct mipi_dsi_device *dsi = boe->dsi;
	struct device *dev = &boe->dsi->dev;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0)
		dev_err(dev, "failed to set display on: %d\n", ret);

	msleep(40);
	return ret;
}

static void boe_panel_off(struct boe_panel *boe)
{
	struct mipi_dsi_device *dsi = boe->dsi;
	struct device *dev = &boe->dsi->dev;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0)
		dev_err(dev, "failed to set display off: %d\n", ret);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0)
		dev_err(dev, "failed to enter sleep mode: %d\n", ret);

	msleep(100);
}

static int boe_panel_disable(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);

	if (!boe->enabled)
		return 0;

	boe->enabled = false;

	return 0;
}

static int boe_panel_unprepare(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);
	struct device *dev = &boe->dsi->dev;
	int ret;

	if (!boe->prepared)
		return 0;

	boe_panel_off(boe);

	gpiod_set_value_cansleep(boe->io_vdd_en_gpio, 0);
	gpiod_set_value_cansleep(boe->vss_pn_en_gpio, 0);
	gpiod_set_value_cansleep(boe->backlight_en_gpio, 0);
	gpiod_set_value_cansleep(boe->n_reset_gpio, 0);
	usleep_range(2000, 3000);

	boe->prepared = false;

	return 0;
}

static int boe_panel_prepare(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);
	struct device *dev = &boe->dsi->dev;
	int ret;

	if (boe->prepared)
		return 0;
	
	gpiod_set_value_cansleep(boe->io_vdd_en_gpio, 0);
	gpiod_set_value_cansleep(boe->vss_pn_en_gpio, 0);
	gpiod_set_value_cansleep(boe->n_reset_gpio, 0);
	usleep_range(2000, 3000);

	gpiod_set_value_cansleep(boe->io_vdd_en_gpio, 1);
	usleep_range(10000, 15000);

	gpiod_set_value_cansleep(boe->vss_pn_en_gpio, 1);
	usleep_range(35000, 40000);

	gpiod_set_value_cansleep(boe->n_reset_gpio, 1);
	usleep_range(15000, 20000);

	ret = boe_panel_init(boe);
	if (ret < 0) {
		dev_err(dev, "failed to init panel: %d\n", ret);
		goto poweroff;
	}

	ret = boe_panel_on(boe);
	if (ret < 0) {
		dev_err(dev, "failed to set panel on: %d\n", ret);
		goto poweroff;
	}

	gpiod_set_value_cansleep(boe->backlight_en_gpio, 1);

	boe->prepared = true;

	return 0;

poweroff:
	gpiod_set_value_cansleep(boe->io_vdd_en_gpio, 0);
	gpiod_set_value_cansleep(boe->vss_pn_en_gpio, 0);
	gpiod_set_value_cansleep(boe->backlight_en_gpio, 0);
	gpiod_set_value_cansleep(boe->n_reset_gpio, 0);
	usleep_range(2000, 3000);

	return ret;
}

static int boe_panel_enable(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);

	if (boe->enabled)
		return 0;

	boe->enabled = true;

	return 0;
}

static const struct drm_display_mode default_mode = {
		// full_clock = 142096860Hz = 142097kHz = htotal*vtotal*60fps
		.clock = 142097,
		.hdisplay = 1440,
		.hsync_start = 1440 + 10,
		.hsync_end = 1440 + 10 + 1,
		// .htotal = hactive + HFP + HSLEN + HBP
		.htotal = 1440 + 10 + 1 + 10,
		.vdisplay = 1600,
		.vsync_start = 1600 + 10,
		.vsync_end = 1600 + 10 + 1,
		// .vtotal = vactive + VFP + VSLEN + VBP
		.vtotal = 1600 + 10 + 1 + 10,
		.flags = 0,
};

static int boe_panel_get_modes(struct drm_panel *panel,
			       struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct boe_panel *boe = to_boe_panel(panel);
	struct device *dev = &boe->dsi->dev;

	mode = drm_mode_duplicate(connector->dev, &default_mode);
	if (!mode) {
		dev_err(dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay, default_mode.vdisplay,
			drm_mode_vrefresh(&default_mode));
		return -ENOMEM;
	}

	drm_mode_set_name(mode);

	drm_mode_probed_add(connector, mode);

	connector->display_info.width_mm = 59;
	connector->display_info.height_mm = 66;

	return 1;
}

static const struct drm_panel_funcs boe_panel_funcs = {
	.disable = boe_panel_disable,
	.unprepare = boe_panel_unprepare,
	.prepare = boe_panel_prepare,
	.enable = boe_panel_enable,
	.get_modes = boe_panel_get_modes,
};

static const struct of_device_id boe_of_match[] = {
	{ .compatible = "boe,vs035zsm-nw0-69p0", },
	{ }
};
MODULE_DEVICE_TABLE(of, boe_of_match);

static int boe_panel_add(struct boe_panel *boe)
{
	struct device *dev = &boe->dsi->dev;
	int ret;
	unsigned int i;

	boe->mode = &default_mode;

	boe->io_vdd_en_gpio = devm_gpiod_get(dev, "io-vdd-en", GPIOD_OUT_LOW);
	if (IS_ERR(boe->io_vdd_en_gpio)) {
		ret = PTR_ERR(boe->io_vdd_en_gpio);
		dev_err(dev, "cannot get io-vdd-en-gpios %d\n", ret);
		return ret;
	}

	boe->n_reset_gpio = devm_gpiod_get(dev, "n-reset", GPIOD_OUT_LOW);
	if (IS_ERR(boe->n_reset_gpio)) {
		ret = PTR_ERR(boe->n_reset_gpio);
		dev_err(dev, "cannot get n-reset-gpios %d\n", ret);
		return ret;
	}

	boe->backlight_en_gpio = devm_gpiod_get(dev, "backlight-en", GPIOD_OUT_LOW);
	if (IS_ERR(boe->backlight_en_gpio)) {
		ret = PTR_ERR(boe->backlight_en_gpio);
		dev_err(dev, "cannot get backlight-en-gpios %d\n", ret);
		return ret;
	}

	boe->vss_pn_en_gpio = devm_gpiod_get(dev, "vss-pn-en", GPIOD_OUT_LOW);
	if (IS_ERR(boe->vss_pn_en_gpio)) {
		ret = PTR_ERR(boe->vss_pn_en_gpio);
		dev_err(dev, "cannot get vss-pn-en-gpios %d\n", ret);
		return ret;
	}

	drm_panel_init(&boe->base, &boe->dsi->dev, &boe_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	drm_panel_add(&boe->base);

	return 0;
}

static void boe_panel_del(struct boe_panel *boe)
{
	if (boe->base.dev)
		drm_panel_remove(&boe->base);
}

static int boe_panel_probe(struct mipi_dsi_device *dsi)
{
	struct boe_panel *boe;
	int ret;

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags =  MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST;

	boe = devm_kzalloc(&dsi->dev, sizeof(*boe), GFP_KERNEL);
	if (!boe)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, boe);

	boe->dsi = dsi;

	ret = boe_panel_add(boe);
	if (ret < 0)
		return ret;

	return mipi_dsi_attach(dsi);
}

static int boe_panel_remove(struct mipi_dsi_device *dsi)
{
	struct boe_panel *boe = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = boe_panel_disable(&boe->base);
	if (ret < 0)
		dev_err(&dsi->dev, "failed to disable panel: %d\n", ret);

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "failed to detach from DSI host: %d\n",
			ret);

	boe_panel_del(boe);

	return 0;
}

static void boe_panel_shutdown(struct mipi_dsi_device *dsi)
{
	struct boe_panel *boe = mipi_dsi_get_drvdata(dsi);

	boe_panel_disable(&boe->base);
}

static struct mipi_dsi_driver boe_panel_driver = {
	.driver = {
		.name = "panel-boe-vs035zsm-nw0-69p0",
		.of_match_table = boe_of_match,
	},
	.probe = boe_panel_probe,
	.remove = boe_panel_remove,
	.shutdown = boe_panel_shutdown,
};
module_mipi_dsi_driver(boe_panel_driver);

MODULE_AUTHOR("Joseph Albers <Joseph.Albers@outlook.de>");
MODULE_DESCRIPTION("BOE VS035ZSM NW0 69P0 1440x1600 video mode panel driver");
MODULE_LICENSE("GPL v2");
