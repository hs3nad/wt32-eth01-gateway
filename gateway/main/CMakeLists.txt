idf_component_register(
  SRCS
    "../src/main.cpp"
    "../src/modbus_tcp.cpp"
    "../src/ota_update.cpp"
    "../src/rcu_client.cpp"
  INCLUDE_DIRS
    "../include"
  REQUIRES
    bootloader_support
    esp_eth
    esp_driver_gpio
    esp_driver_i2c
    esp_driver_uart
    esp_event
    esp_http_client
    esp_https_ota
    esp_netif
    esp_timer
    lwip
    mbedtls
    nvs_flash
)

idf_component_get_property(lwip_lib lwip COMPONENT_LIB)
target_compile_definitions(${lwip_lib} PRIVATE LWIP_NETBIOS_RESPOND_NAME_QUERY=1)
