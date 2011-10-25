
#define TOUCH_POWER_CONTROL

#ifdef TOUCH_POWER_CONTROL

#define TOUCH_IDLE_MODE 0x4D
#define TOUCH_NORMAL_MODE 0x4E
#define TOUCH_QUERY_MODE 0x4F
//&*&*&*AL1_20100222, enhanced command response ratio
#define TOUCH_REGION_CMD 0x4C
//&*&*&*AL1_20100222, enhanced command response ratio

enum hanvon_idle_mode {
	D0=0,
	D1,
	D2,
	D3,
};
struct hanvon_idle_aa{
	unsigned long  aa_region[6];
};
//&*&*&*AL1_20100222, enhanced command response ratio
struct cmd_send{
	unsigned char cmd;
	int send_count;
	wait_queue_head_t wait_rsp;
};
//&*&*&*AL2_20100222, enhanced command response ratio
struct touch_power{
	struct serio* serio;
	struct hanvon_idle_aa hanvon_idle;
	struct delayed_work	delay_work_idle;
	int start_idle;
	int wait_idle_cmd_response;
	spinlock_t spinlock;
	int  (*transaction)(struct serio *serio,
				     unsigned char data, unsigned int flags);
	int transaction_mode;
	//&*&*&*AL1_20100222, enhanced command response ratio
	struct timer_list send_cmd_timeout;
	struct cmd_send send_cmd;
	int suspend;
	//&*&*&*AL2_20100222, enhanced command response ratio
};

extern int touch_power_init(struct serio_driver* serio_drv, struct serio* serio);
extern int touch_power_deinit(struct serio_driver* serio_drv);
extern int touch_power_register(struct serio_driver* serio_drv);
extern void touch_power_unregister(struct serio_driver* serio_drv);

#endif






