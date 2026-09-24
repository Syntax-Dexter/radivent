set(thread_manager_requires app_config)

if(CONFIG_FANCTRL_ENABLE_THREAD AND CONFIG_OPENTHREAD_ENABLED)
    list(APPEND thread_manager_requires openthread)
endif()

idf_component_register(
    SRCS "thread_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES ${thread_manager_requires}
)
