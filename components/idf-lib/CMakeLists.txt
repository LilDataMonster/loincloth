# embed ca_cert certificate into binary
if(CONFIG_OTA_ENABLED)
    set(cert_pem ${project_dir}/server_certs/ca_cert.pem)
else()
    set(cert_pem "")
endif()

idf_component_register(
    SRC_DIRS src
    INCLUDE_DIRS include
    REQUIRES bt
             json
             esp-tls
             esp_http_client
             esp_https_ota
    EMBED_TXTFILES ${cert_pem}
)
