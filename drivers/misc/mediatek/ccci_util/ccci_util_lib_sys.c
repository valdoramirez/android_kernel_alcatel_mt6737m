
#include <mt-plat/sync_write.h>
#include <mt-plat/mt_ccci_common.h>
#include <linux/slab.h>
#include <linux/kobject.h>
#include "ccci_util_log.h"
#include "ccci_util_lib_sys.h"

#define CCCI_KOBJ_NAME "ccci"

struct ccci_info {
	struct kobject kobj;
	unsigned int ccci_attr_count;
};

struct ccci_attribute {
	struct attribute attr;
	ssize_t (*show)(char *buf);
	ssize_t (*store)(const char *buf, size_t count);
};

#define CCCI_ATTR(_name, _mode, _show, _store)			\
static struct ccci_attribute ccci_attr_##_name = {		\
	.attr = {.name = __stringify(_name), .mode = _mode },	\
	.show = _show,						\
	.store = _store,					\
}

static struct ccci_info *ccci_sys_info;

static void ccci_obj_release(struct kobject *kobj)
{
	struct ccci_info *ccci_info_temp = container_of(kobj, struct ccci_info, kobj);

	kfree(ccci_info_temp);
	ccci_sys_info = NULL;	/* as ccci_info_temp==ccci_sys_info */
}

static ssize_t ccci_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	ssize_t len = 0;

	struct ccci_attribute *a = container_of(attr, struct ccci_attribute, attr);

	if (a->show)
		len = a->show(buf);

	return len;
}

static ssize_t ccci_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	ssize_t len = 0;

	struct ccci_attribute *a = container_of(attr, struct ccci_attribute, attr);

	if (a->store)
		len = a->store(buf, count);

	return len;
}

static const struct sysfs_ops ccci_sysfs_ops = {
	.show = ccci_attr_show,
	.store = ccci_attr_store
};

/*======================================= */
/* CCCI common sys attribute part */
/*======================================= */
/* Sys -- boot status */
static get_status_func_t get_status_func[MAX_MD_NUM];
static boot_md_func_t boot_md_func[MAX_MD_NUM];
static int get_md_status(int md_id, char val[], int size)
{
	if ((md_id < MAX_MD_NUM) && (get_status_func[md_id] != NULL))
		(get_status_func[md_id]) (md_id, val, size);
	else
		snprintf(val, 32, "md%d:n/a", md_id + 1);

	return 0;
}

static int trigger_md_boot(int md_id)
{
	if ((md_id < MAX_MD_NUM) && (boot_md_func[md_id] != NULL)) {
		(boot_md_func[md_id]) (md_id);
		return 0;
	}

	return -1;
}

static ssize_t boot_status_show(char *buf)
{
	char md1_sta_str[32];
	char md2_sta_str[32];
	char md3_sta_str[32];
	char md5_sta_str[32];

	/* MD1 --- */
	get_md_status(MD_SYS1, md1_sta_str, 32);
	/* MD2 --- */
	get_md_status(MD_SYS2, md2_sta_str, 32);
	/* MD3 */
	get_md_status(MD_SYS3, md3_sta_str, 32);
	/* MD5 */
	get_md_status(MD_SYS5, md5_sta_str, 32);

	/* Final string */
	return snprintf(buf, 32 * 4 + 3 * 4 + 1, "%s | %s | %s | md4:n/a | %s\n", md1_sta_str, md2_sta_str, md3_sta_str,
			md5_sta_str);
}

static ssize_t boot_status_store(const char *buf, size_t count)
{
	unsigned int md_id;

	md_id = buf[0] - '0';
	CCCI_UTIL_INF_MSG("md%d get boot store\n", md_id + 1);
	if (md_id < MAX_MD_NUM) {
		if (trigger_md_boot(md_id) != 0)
			CCCI_UTIL_INF_MSG("md%d n/a\n", md_id + 1);
	} else
		CCCI_UTIL_INF_MSG("invalid id(%d)\n", md_id + 1);
	return count;
}

CCCI_ATTR(boot, 0660, &boot_status_show, &boot_status_store);

/* Sys -- enable status */
static ssize_t ccci_md_enable_show(char *buf)
{
	int i;
	char md_en[MAX_MD_NUM];

	for (i = 0; i < MAX_MD_NUM; i++) {
		if (get_modem_is_enabled(MD_SYS1 + i))
			md_en[i] = 'E';
		else
			md_en[i] = 'D';
	}

	/* Final string */
	return snprintf(buf, 32, "%c-%c-%c-%c-%c (1->5)\n", md_en[0], md_en[1], md_en[2], md_en[3], md_en[4]);
}

CCCI_ATTR(md_en, 0660, &ccci_md_enable_show, NULL);

/* Sys -- Versin */
static unsigned int ccci_port_ver = 3;
static ssize_t ccci_version_show(char *buf)
{
	return snprintf(buf, 16, "%d\n", ccci_port_ver);	/* ECCCI */
}

void update_ccci_port_ver(unsigned int new_ver)
{
	ccci_port_ver = new_ver;
}

CCCI_ATTR(version, 0644, &ccci_version_show, NULL);

static ssize_t debug_enable_show(char *buf)
{
	int curr = 0;

	curr = snprintf(buf, 16, "%d\n", ccci_debug_enable);
	return curr;
}

static ssize_t debug_enable_store(const char *buf, size_t count)
{
	ccci_debug_enable = buf[0] - '0';
	return count;
}

CCCI_ATTR(debug, 0660, &debug_enable_show, &debug_enable_store);

static ssize_t kcfg_setting_show(char *buf)
{
	unsigned int curr = 0;
	unsigned int actual_write;
	char md_en[MAX_MD_NUM];
	unsigned int md_num = 0;
	int i;

	for (i = 0; i < MAX_MD_NUM; i++) {
		if (get_modem_is_enabled(MD_SYS1 + i)) {
			md_num++;
			md_en[i] = '1';
		} else
			md_en[i] = '0';
	}
	/* MD enable setting part */
	/* Reserve 16 byte to store size info */
	actual_write = snprintf(&buf[curr], 4096 - 16 - curr, "[modem num]:%d\n", md_num);
	curr += actual_write;
	/* Reserve 16 byte to store size info */
	actual_write = snprintf(&buf[curr], 4096 - 16 - curr,
		"[modem en]:%c-%c-%c-%c-%c\n", md_en[0], md_en[1], md_en[2], md_en[3], md_en[4]);
	curr += actual_write;

	/* Feature option part */
#ifdef CONFIG_EVDO_DT_SUPPORT
	actual_write = snprintf(&buf[curr], 4096 - curr, "[EVDO_DT_SUPPORT]:1\n");
#else
	actual_write = snprintf(&buf[curr], 4096 - curr, "[EVDO_DT_SUPPORT]:0\n");
#endif
	curr += actual_write;
#ifdef CONFIG_MTK_LTE_DC_SUPPORT
	actual_write = snprintf(&buf[curr], 4096 - curr, "[MTK_LTE_DC_SUPPORT]:1\n");
#else
	actual_write = snprintf(&buf[curr], 4096 - curr, "[MTK_LTE_DC_SUPPORT]:0\n");
#endif
	curr += actual_write;
#ifdef CONFIG_MTK_ECCCI_C2K
	actual_write = snprintf(&buf[curr], 4096 - curr, "[MTK_ECCCI_C2K]:1\n");
	curr += actual_write;
#endif
	/* Add total size to tail */
	actual_write = snprintf(&buf[curr], 4096 - curr, "total:%d\n", curr);
	curr += actual_write;

	CCCI_UTIL_INF_MSG("cfg_info_buffer size:%d\n", curr);
	return (ssize_t) curr;
}

static ssize_t kcfg_setting_store(const char *buf, size_t count)
{
	return count;
}

CCCI_ATTR(kcfg_setting, 0444, &kcfg_setting_show, &kcfg_setting_store);

/* Sys -- Add to group */
static struct attribute *ccci_default_attrs[] = {
	&ccci_attr_boot.attr,
	&ccci_attr_version.attr,
	&ccci_attr_md_en.attr,
	&ccci_attr_debug.attr,
	&ccci_attr_kcfg_setting.attr,
	NULL
};

static struct kobj_type ccci_ktype = {
	.release = ccci_obj_release,
	.sysfs_ops = &ccci_sysfs_ops,
	.default_attrs = ccci_default_attrs
};

int ccci_sysfs_add_modem(int md_id, void *kobj, void *ktype, get_status_func_t get_sta_func, boot_md_func_t boot_func)
{
	int ret;
	static int md_add_flag;

	md_add_flag = 0;
	if (!ccci_sys_info) {
		CCCI_UTIL_ERR_MSG("common sys not ready\n");
		return -CCCI_ERR_SYSFS_NOT_READY;
	}

	if (md_add_flag & (1 << md_id)) {
		CCCI_UTIL_ERR_MSG("md%d sys dup add\n", md_id + 1);
		return -CCCI_ERR_SYSFS_NOT_READY;
	}

	ret =
	    kobject_init_and_add((struct kobject *)kobj, (struct kobj_type *)ktype, &ccci_sys_info->kobj, "mdsys%d",
				 md_id + 1);
	if (ret < 0) {
		kobject_put(kobj);
		CCCI_UTIL_ERR_MSG_WITH_ID(md_id, "fail to add md kobject\n");
	} else {
		md_add_flag |= (1 << md_id);
		get_status_func[md_id] = get_sta_func;
		boot_md_func[md_id] = boot_func;
	}

	return ret;
}

int ccci_common_sysfs_init(void)
{
	int ret = 0;
	int i;

	ccci_sys_info = kmalloc(sizeof(struct ccci_info), GFP_KERNEL);
	if (!ccci_sys_info)
		return -ENOMEM;

	memset(ccci_sys_info, 0, sizeof(struct ccci_info));

	ret = kobject_init_and_add(&ccci_sys_info->kobj, &ccci_ktype, kernel_kobj, CCCI_KOBJ_NAME);
	if (ret < 0) {
		kobject_put(&ccci_sys_info->kobj);
		CCCI_UTIL_ERR_MSG("fail to add ccci kobject\n");
		return ret;
	}
	for (i = 0; i < MAX_MD_NUM; i++) {
		get_status_func[i] = NULL;
		boot_md_func[i] = NULL;
	}

	ccci_sys_info->ccci_attr_count = ARRAY_SIZE(ccci_default_attrs) - 1;
	CCCI_UTIL_DBG_MSG("ccci attr cnt %d\n", ccci_sys_info->ccci_attr_count);
	return ret;
}
