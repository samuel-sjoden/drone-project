idf_component_register(SRCS "app_main.c" 
                            "foc/foc.c"           
                            "inverter/inverter.c"
                       REQUIRES driver
                       INCLUDE_DIRS "foc" "inverter" "." 
)

