
ifeq ($(CONFIG_BUILD_ASD_LIB),y)
ifneq ($(wildcard $(srctree)/$(src)/asdlib),)
obj-y += asdlib/
endif
endif

ifneq ($(wildcard $(srctree)/$(src)/applib/src),)
obj-$(CONFIG_APP_CONNECTED_APPLIB) += applib/
endif

ifneq ($(wildcard $(srctree)/$(src)/app),)
obj-$(CONFIG_APP_CONNECTED_APP) += app/
endif
