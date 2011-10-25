obj-$(CONFIG_FB_EINK_HAL)            += eink_fb_hal.o
eink_fb_hal-objs                     := einkfb_hal_main.o               \
                                        einkfb_hal_events.o             \
                                        einkfb_hal_util.o               \
                                        einkfb_hal_proc.o               \
                                        einkfb_hal_mem.o                \
                                        einkfb_hal_io.o                 \
                                        einkfb_hal_pm.o

obj-$(CONFIG_FB_EINK_HAL_EMULATOR)   += eink_fb_hal_emu.o 
eink_fb_hal_emu-objs                 := ../emulator/emulator_hal.o

obj-$(CONFIG_FB_EINK_HAL_APOLLO)     += eink_fb_hal_apollo.o 
eink_fb_hal_apollo-objs              := ../apollo/apollo_hal.o          \
                                        ../apollo/apollo_kernel.o       \
                                        ../apollo/apollo.o

obj-$(CONFIG_FB_EINK_HAL_STEPHANIE)  += eink_fb_hal_steph.o 
eink_fb_hal_steph-objs               := ../stephanie/stephanie_hal.o    \
                                        ../apollo/apollo_kernel.o       \
                                        ../apollo/apollo_universal.o

obj-$(CONFIG_FB_EINK_HAL_EIGHTRACK)  += eink_fb_hal_8track.o 
eink_fb_hal_8track-objs              := ../eightrack/eightrack_hal.o    \
                                        ../eightrack/eightrack.o

obj-$(CONFIG_FB_EINK_HAL_BROADSHEET) += eink_fb_hal_broads.o 
eink_fb_hal_broads-objs              := ../broadsheet/broadsheet_hal.o  \
                                        ../broadsheet/broadsheet_mxc.o  \
                                        ../broadsheet/broadsheet.o	\
                                        ../broadsheet/hirq_x3.o

obj-$(CONFIG_FB_EINK_HAL_ISIS)       += eink_fb_hal_isis.o 
eink_fb_hal_isis-objs                := ../isis/broadsheet_hal.o        \
                                        ../isis/broadsheet_mxc.o        \
                                        ../isis/broadsheet.o

# only for testing!

obj-$(CONFIG_FB_EINK_HAL_BUILTIN_EM) += eink_fb_hal_builtin_em.o
eink_fb_hal_builtin_em-objs          := einkfb_hal_builtin_base.o       \
                                        ../emulator/builtin_em.o

obj-$(CONFIG_FB_EINK_HAL_BUILTIN_BS) += eink_fb_hal_builtin_bs.o
eink_fb_hal_builtin_bs-objs          := einkfb_hal_builtin_base.o       \
                                        ../broadsheet/builtin_bs.o
                                        
