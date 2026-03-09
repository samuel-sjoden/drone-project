idf_component_register(SRCS "app_main.c" 
                            "foc/foc.c"           
                            "inverter/inverter.c"
							"foc/sin_lut.c"
                       REQUIRES driver
                       INCLUDE_DIRS "foc" "inverter" "." 
)

target_compile_options(${COMPONENT_LIB} PRIVATE "-save-temps" "-fverbose-asm")

