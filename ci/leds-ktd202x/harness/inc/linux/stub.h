#pragma once
#include <stddef.h>
#include <stdbool.h>
typedef unsigned char u8; typedef unsigned int u32; typedef unsigned long kernel_ulong_t;
#define BIT(n) (1UL << (n))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#define min_t(t, a, b) ((t)(a) < (t)(b) ? (t)(a) : (t)(b))
#define clamp(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))
#define DIV_ROUND_CLOSEST(x, d) (((x) + ((d) / 2)) / (d))
#define struct_size(p, m, n) (sizeof(*(p)) + sizeof((p)->m[0]) * (n))
#define __counted_by(x)
#define GFP_KERNEL 0
#define ENOMEM 12
#define EINVAL 22
#define ENODEV 19
#define IS_ERR(p) ((unsigned long)(p) > (unsigned long)-4096)
#define PTR_ERR(p) ((long)(p))
struct device { int x; }; struct fwnode_handle;
struct mutex { int x; };
void mutex_lock(struct mutex *); void mutex_unlock(struct mutex *);
int devm_mutex_init(struct device *, struct mutex *);
struct regmap;
struct reg_default { unsigned int reg, def; };
enum regcache_type { REGCACHE_NONE, REGCACHE_FLAT };
struct regmap_config { int reg_bits, val_bits; unsigned int max_register; enum regcache_type cache_type; const struct reg_default *reg_defaults; unsigned int num_reg_defaults; };
int regmap_write(struct regmap *, unsigned int, unsigned int);
int regmap_read(struct regmap *, unsigned int, unsigned int *);
int regmap_update_bits(struct regmap *, unsigned int, unsigned int, unsigned int);
struct regulator_bulk_data { const char *supply; void *consumer; };
int regulator_bulk_enable(int, struct regulator_bulk_data *);
int regulator_bulk_disable(int, struct regulator_bulk_data *);
int devm_regulator_bulk_get(struct device *, int, struct regulator_bulk_data *);
void dev_err(struct device *, const char *, ...);
int dev_err_probe(struct device *, int, const char *, ...);
void *devm_kzalloc(struct device *, size_t, int);
void *devm_kcalloc(struct device *, size_t, size_t, int);
void usleep_range(unsigned long, unsigned long);
unsigned int device_get_child_node_count(struct device *);
struct fwnode_handle *fwnode_get_next_child_node(struct fwnode_handle *, struct fwnode_handle *);
struct fwnode_handle *device_get_next_child_node(struct device *, struct fwnode_handle *);
#define fwnode_for_each_available_child_node(fw, c) for (c = fwnode_get_next_child_node(fw, NULL); c; c = fwnode_get_next_child_node(fw, c))
#define device_for_each_child_node_scoped(dev, c) for (struct fwnode_handle *c = device_get_next_child_node(dev, NULL); c; c = device_get_next_child_node(dev, c))
int fwnode_property_read_u32(struct fwnode_handle *, const char *, u32 *);
void fwnode_handle_put(struct fwnode_handle *);
enum led_brightness { LED_OFF = 0, LED_FULL = 255 };
#define LED_COLOR_ID_RGB 9
struct led_pattern { u32 delta_t; int brightness; };
struct led_classdev { const char *name; unsigned int brightness; unsigned int max_brightness;
	int (*brightness_set_blocking)(struct led_classdev *, enum led_brightness);
	int (*blink_set)(struct led_classdev *, unsigned long *, unsigned long *);
	int (*pattern_set)(struct led_classdev *, struct led_pattern *, u32, int);
	int (*pattern_clear)(struct led_classdev *); };
struct mc_subled { unsigned int color_index, brightness, intensity, channel; };
struct led_classdev_mc { struct led_classdev led_cdev; struct mc_subled *subled_info; unsigned int num_colors; };
#define lcdev_to_mccdev(c) container_of(c, struct led_classdev_mc, led_cdev)
struct led_init_data { struct fwnode_handle *fwnode; };
int led_mc_calc_color_components(struct led_classdev_mc *, enum led_brightness);
int devm_led_classdev_multicolor_register_ext(struct device *, struct led_classdev_mc *, struct led_init_data *);
int devm_led_classdev_register_ext(struct device *, struct led_classdev *, struct led_init_data *);
struct i2c_client { struct device dev; };
struct i2c_device_id { char name[20]; kernel_ulong_t driver_data; };
struct of_device_id { char compatible[128]; const void *data; };
struct device_driver { const char *name; const struct of_device_id *of_match_table; };
struct i2c_driver { struct device_driver driver; int (*probe)(struct i2c_client *); void (*remove)(struct i2c_client *); void (*shutdown)(struct i2c_client *); const struct i2c_device_id *id_table; };
const void *i2c_get_match_data(const struct i2c_client *);
void i2c_set_clientdata(struct i2c_client *, void *);
void *i2c_get_clientdata(const struct i2c_client *);
struct regmap *devm_regmap_init_i2c(struct i2c_client *, const struct regmap_config *);
#define MODULE_DEVICE_TABLE(a, b)
#define module_i2c_driver(d)
#define MODULE_AUTHOR(x)
#define MODULE_DESCRIPTION(x)
#define MODULE_LICENSE(x)
#define clamp_t(t, v, lo, hi) clamp((t)(v), (t)(lo), (t)(hi))
#define fwnode_for_each_child_node(fw, c) fwnode_for_each_available_child_node(fw, c)
#define max(a, b) ((a) > (b) ? (a) : (b))
#define EBUSY 16
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define check_add_overflow(a, b, d) __builtin_add_overflow(a, b, d)
#define ULONG_MAX (~0UL)
typedef unsigned long long u64;
static inline u64 div64_u64(u64 a, u64 b) { return a / b; }
static inline u64 mul_u64_u64_div_u64(u64 a, u64 m, u64 d) { return (u64)((unsigned __int128)a * m / d); }
