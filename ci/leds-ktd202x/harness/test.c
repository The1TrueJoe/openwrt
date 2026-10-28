/* Userspace harness: runs leds-ktd202x.c against a simulated register file. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <linux/stub.h>

struct fwnode_handle { u32 reg; int color; u32 ua; struct fwnode_handle *kids; int nk; };
#define ENXIO 6

static u8 hw[16], cache[16];
static int writes[16];
static int fail_io;
static const void *match;
static struct fwnode_handle kids[4];
static int nkids;
static struct led_classdev *leds[4];
static int nleds;

void mutex_lock(struct mutex *m) { }
void mutex_unlock(struct mutex *m) { }
int devm_mutex_init(struct device *d, struct mutex *m) { return 0; }
int regulator_bulk_enable(int n, struct regulator_bulk_data *r) { return 0; }
int regulator_bulk_disable(int n, struct regulator_bulk_data *r) { return 0; }
int devm_regulator_bulk_get(struct device *d, int n, struct regulator_bulk_data *r) { return 0; }
void dev_err(struct device *d, const char *f, ...) { va_list a; va_start(a, f); printf("    dev_err: "); vprintf(f, a); va_end(a); }
int dev_err_probe(struct device *d, int e, const char *f, ...) { va_list a; va_start(a, f); printf("    dev_err_probe(%d): ", e); vprintf(f, a); va_end(a); return e; }
void *devm_kzalloc(struct device *d, size_t s, int g) { return calloc(1, s); }
void *devm_kcalloc(struct device *d, size_t n, size_t s, int g) { return calloc(n, s); }
void usleep_range(unsigned long a, unsigned long b) { }
unsigned int device_get_child_node_count(struct device *d) { return nkids; }
struct fwnode_handle *device_get_next_child_node(struct device *d, struct fwnode_handle *c)
{ int i = c ? (int)(c - kids) + 1 : 0; return i < nkids ? &kids[i] : NULL; }
struct fwnode_handle *fwnode_get_next_child_node(struct fwnode_handle *f, struct fwnode_handle *c)
{ int i = c ? (int)(c - f->kids) + 1 : 0; return i < f->nk ? &f->kids[i] : NULL; }
int fwnode_property_read_u32(struct fwnode_handle *f, const char *n, u32 *v)
{
	if (!strcmp(n, "reg")) { *v = f->reg; return 0; }
	if (!strcmp(n, "color")) { if (f->color < 0) return -EINVAL; *v = f->color; return 0; }
	if (!strcmp(n, "led-max-microamp")) { if (!f->ua) return -EINVAL; *v = f->ua; return 0; }
	return -EINVAL;
}
void fwnode_handle_put(struct fwnode_handle *f) { }
int led_mc_calc_color_components(struct led_classdev_mc *m, enum led_brightness b)
{
	for (unsigned int i = 0; i < m->num_colors; i++)
		m->subled_info[i].brightness = DIV_ROUND_CLOSEST(b * m->subled_info[i].intensity, m->led_cdev.max_brightness);
	return 0;
}
int devm_led_classdev_multicolor_register_ext(struct device *d, struct led_classdev_mc *m, struct led_init_data *i)
{ if (!m->led_cdev.max_brightness) m->led_cdev.max_brightness = 255; leds[nleds++] = &m->led_cdev; return 0; }
int devm_led_classdev_register_ext(struct device *d, struct led_classdev *c, struct led_init_data *i)
{ if (!c->max_brightness) c->max_brightness = 255; leds[nleds++] = c; return 0; }
const void *i2c_get_match_data(const struct i2c_client *c) { return match; }
static void *clientdata;
void i2c_set_clientdata(struct i2c_client *c, void *p) { clientdata = p; }
void *i2c_get_clientdata(const struct i2c_client *c) { return clientdata; }
static const struct regmap_config *rcfg;
struct regmap *devm_regmap_init_i2c(struct i2c_client *c, const struct regmap_config *cfg)
{
	rcfg = cfg;
	memset(cache, 0, sizeof(cache));
	for (unsigned int i = 0; i < cfg->num_reg_defaults; i++)
		cache[cfg->reg_defaults[i].reg] = cfg->reg_defaults[i].def;
	memcpy(hw, cache, sizeof(hw));
	return (struct regmap *)1;
}
int regmap_write(struct regmap *m, unsigned int r, unsigned int v)
{
	if (fail_io) return -ENXIO;
	if (r > rcfg->max_register) { printf("    BAD REG %u\n", r); abort(); }
	if (v > 0xff) { printf("    VALUE OVERFLOW reg %u = %u\n", r, v); abort(); }
	hw[r] = cache[r] = v; writes[r]++;
	return 0;
}
int regmap_read(struct regmap *m, unsigned int r, unsigned int *v) { *v = cache[r]; return 0; }
int regmap_update_bits(struct regmap *m, unsigned int r, unsigned int mask, unsigned int v)
{
	unsigned int t = (cache[r] & ~mask) | (v & mask);
	return t == cache[r] ? 0 : regmap_write(m, r, t);
}

#include "leds-ktd202x.c"

static int fails;
#define CHECK(cond, ...) do { if (cond) printf("  ok   " __VA_ARGS__); else { fails++; printf("  FAIL " __VA_ARGS__); } printf("\n"); } while (0)

static int probe(const struct ktd202x_info *info, int n, u32 ua)
{
	static struct i2c_client client;
	memset(writes, 0, sizeof(writes)); nleds = 0; match = info; nkids = n;
	for (int i = 0; i < n; i++) kids[i] = (struct fwnode_handle){ .reg = i, .color = 3 + i, .ua = ua };
	return ktd202x_probe(&client);
}

static int blink(int l, unsigned long on, unsigned long off, unsigned long *ron, unsigned long *roff)
{
	*ron = on; *roff = off;
	return leds[l]->blink_set(leds[l], ron, roff);
}

#define CH(n) ((hw[4] >> (2 * (n))) & 3)

int main(void) __attribute__((used));
int main(void)
{
	unsigned long on, off;
	int ret;

	setvbuf(stdout, NULL, _IONBF, 0);

	printf("probe ET6326 with 2 of 3 LEDs (guard malloc catches the OOB read)\n");
	ret = probe(&et6326_info, 2, 1375);
	CHECK(ret == 0, "probe returns %d", ret);
	CHECK(nleds == 2 && leds[0]->max_brightness == 11, "2 LEDs, max_brightness %u (1375 uA)", leds[0]->max_brightness);

	printf("brightness\n");
	leds[0]->brightness_set_blocking(leds[0], 11);
	CHECK(hw[6] == 0x0a && CH(0) == 1, "blue at 11: IOUT 0x%02x (stock 0x0a), mode %d", hw[6], CH(0));
	leds[0]->brightness_set_blocking(leds[0], 0);
	CHECK(CH(0) == 0 && hw[0] == KTD202X_ENABLE_CTRL_SLEEP, "blue off: mode %d, chip asleep (Reg0 0x%02x)", CH(0), hw[0]);

	printf("blink, one LED (ET6326 period encoding)\n");
	ret = blink(0, 500, 500, &on, &off);
	CHECK(ret == 0 && hw[1] == 6 && hw[2] == 128 && CH(0) == 2, "500/500: ret %d Reg1 %u Reg2 %u mode %d", ret, hw[1], hw[2], CH(0));
	CHECK(on == 448 && off == 448, "reported %lu/%lu (896 ms period)", on, off);
	CHECK(hw[6] == 0x0a, "blink from off uses max_brightness: IOUT 0x%02x", hw[6]);

	printf("O13: one flash timer; a second LED never retimes the first\n");
	int r1 = writes[1], r2 = writes[2];
	ret = blink(1, 100, 900, &on, &off);
	CHECK(ret == -EBUSY && CH(1) == 1 && on == 100, "white 100/900 (other duty): ret %d, white steady (mode %d), delays untouched %lu", ret, CH(1), on);
	CHECK(hw[2] == 128 && CH(0) == 2 && writes[1] == r1 && writes[2] == r2, "blue untouched: Reg2 %u mode %d, Reg1/Reg2 not rewritten", hw[2], CH(0));
	ret = blink(1, 200, 200, &on, &off);
	CHECK(ret == -EBUSY && CH(1) == 1, "white 200/200 (other period): ret %d, steady", ret);
	ret = blink(1, 500, 500, &on, &off);
	CHECK(ret == 0 && CH(1) == 2 && CH(0) == 2 && writes[1] == r1 && writes[2] == r2, "white 500/500 (same timing): ret %d, shares the timer (mode %d), nothing rewritten", ret, CH(1));
	CHECK(hw[3] == 1, "Reg3 (PWM2 on time) never used: 0x%02x", hw[3]);

	printf("O13: pattern needs ramps the blinking LED does not use -> -EBUSY\n");
	struct led_pattern p[4] = { { 1920, 0 }, { 384, 11 }, { 1920, 11 }, { 0, 0 } };
	ret = leds[1]->pattern_set(leds[1], p, 4, -1);
	CHECK(ret == -EBUSY && CH(1) == 0, "white breathe: ret %d, white off (mode %d)", ret, CH(1));

	printf("turning blue off frees its timer\n");
	leds[0]->brightness_set_blocking(leds[0], 0);
	CHECK(CH(0) == 0, "blue off");
	ret = leds[1]->pattern_set(leds[1], p, 4, -1);
	CHECK(ret == 0 && hw[1] == 32 && hw[5] == 0xff && CH(1) == 2 && hw[2] == 140,
	      "white breathes like stock: ret %d Reg1 %u Reg5 0x%02x Reg2 %u mode %d", ret, hw[1], hw[5], hw[2], CH(1));
	ret = blink(0, 500, 500, &on, &off);
	CHECK(ret == -EBUSY, "blue blink while white breathes: ret %d (software fallback)", ret);
	leds[1]->pattern_clear(leds[1]);
	CHECK(CH(1) == 0, "pattern_clear: white off");

	printf("O14: tiny duty is never programmed as 0\n");
	ret = blink(0, 1, 2000, &on, &off);
	CHECK(ret == 0 && hw[2] == 1 && on > 0, "1/2000: ret %d Reg2 %u reported on %lu off %lu", ret, hw[2], on, off);
	leds[0]->brightness_set_blocking(leds[0], 0);

	printf("hostile delays from sysfs\n");
	ret = blink(0, 1UL << 63, 1UL << 63, &on, &off);
	CHECK(ret == 0 && hw[1] == 127 && hw[2] == 128, "2^63 + 2^63 (sum wraps to 0): ret %d Reg1 %u Reg2 %u, no division by zero", ret, hw[1], hw[2]);
	ret = blink(0, ~0UL, 1, &on, &off);
	CHECK(ret == 0 && hw[2] == 255, "ULONG_MAX on, 1 off: Reg2 %u (longest on time)", hw[2]);
	ret = blink(0, 20000000, 20000000, &on, &off);
	CHECK(ret == 0 && hw[1] == 127 && hw[2] == 128, "5.5 h on, 5.5 h off: Reg1 %u Reg2 %u", hw[1], hw[2]);
	leds[0]->brightness_set_blocking(leds[0], 0);

	printf("rejected patterns\n");
	struct led_pattern bad[4] = { { 100, 5 }, { 100, 11 }, { 100, 11 }, { 100, 0 } };
	CHECK(leds[0]->pattern_set(leds[0], bad, 4, -1) == -EINVAL, "first brightness not 0");
	CHECK(leds[0]->pattern_set(leds[0], p, 3, -1) == -EINVAL, "length 3");
	CHECK(leds[0]->pattern_set(leds[0], p, 4, 3) == -EINVAL, "repeat 3");
	struct led_pattern hot[4] = { { 100, 0 }, { 100, 12 }, { 100, 12 }, { 100, 0 } };
	CHECK(leds[0]->pattern_set(leds[0], hot, 4, -1) == -EINVAL, "brightness 12 > max 11");

	printf("the pattern trigger passes repeat 0 until repeat is written\n");
	ret = leds[0]->pattern_set(leds[0], p, 4, 0);
	CHECK(ret == 0 && CH(0) == 2 && hw[5] == 0xff, "repeat 0: ret %d mode %d Reg5 0x%02x", ret, CH(0), hw[5]);
	leds[0]->pattern_clear(leds[0]);

	printf("KTD2026 keeps its own period encoding\n");
	ret = probe(&ktd2026_info, 3, 0);
	CHECK(ret == 0 && leds[2]->max_brightness == 192, "probe, max %u", leds[2]->max_brightness);
	ret = blink(2, 500, 500, &on, &off);
	CHECK(ret == 0 && hw[1] == 5 && on + off == 896, "500/500: Reg1 %u period %lu", hw[1], on + off);
	ret = blink(2, 1, 1, &on, &off);
	CHECK(ret == 0 && hw[1] == 0, "1/1 (period < 256 ms): Reg1 %u", hw[1]);

	printf("multicolor: led-max-microamp per color, unbounded intensities\n");
	static struct fwnode_handle rgbk[3] = { { .reg = 0, .color = 1, .ua = 1375 }, { .reg = 1, .color = 2 }, { .reg = 2, .color = 3, .ua = 2500 } };
	nleds = 0; match = &ktd2026_info; nkids = 1;
	kids[0] = (struct fwnode_handle){ .color = LED_COLOR_ID_RGB, .kids = rgbk, .nk = 3 };
	static struct i2c_client mcclient;
	ret = ktd202x_probe(&mcclient);
	struct led_classdev_mc *mc = lcdev_to_mccdev(leds[0]);
	CHECK(ret == 0 && nleds == 1 && leds[0]->max_brightness == 192, "probe RGB, max %u", leds[0]->max_brightness);
	leds[0]->brightness_set_blocking(leds[0], 192);
	CHECK(hw[6] == 10 && hw[7] == 191 && hw[8] == 19, "full: IOUT %u/%u/%u (caps 1375 uA, none, 2500 uA)", hw[6], hw[7], hw[8]);
	leds[0]->brightness_set_blocking(leds[0], 96);
	CHECK(hw[6] == 5 && hw[7] == 95 && hw[8] == 9, "half: IOUT %u/%u/%u", hw[6], hw[7], hw[8]);
	mc->subled_info[0].intensity = 100000; mc->subled_info[1].intensity = (unsigned int)-5;
	leds[0]->brightness_set_blocking(leds[0], 192);
	CHECK(hw[6] <= 10 && hw[7] <= 191, "intensity 100000 and -5: IOUT %u/%u stay within the caps", hw[6], hw[7]);
	mc->subled_info[0].intensity = 1;
	leds[0]->brightness_set_blocking(leds[0], 192);
	CHECK(hw[6] == 0 && CH(0) == 1, "intensity 1 still lights (0.125 mA): IOUT %u mode %d", hw[6], CH(0));

	printf("absent chip\n");
	fail_io = 1;
	ret = probe(&et6326_info, 2, 0);
	fail_io = 0;
	CHECK(ret == -ENXIO && nleds == 0, "probe returns %d, %d LEDs registered", ret, nleds);

	printf("%s: %d failure(s)\n", fails ? "FAILED" : "PASSED", fails);
	return fails != 0;
}
