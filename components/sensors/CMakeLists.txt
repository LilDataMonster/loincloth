idf_component_register(
    SRCS "src/dht.cpp"
         "src/bme680.cpp"
    INCLUDE_DIRS include/
    REQUIRES "dht"
             "bme680"
             "json"
)
