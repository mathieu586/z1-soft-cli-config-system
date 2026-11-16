#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "esp_system.h"
#include "esp_console.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "argtable3/argtable3.h"


static struct {
    struct arg_str *key;
    struct arg_str *val;
    struct arg_end *end;
} set_args;

static int set_cmd(int argc, char **argv) {
    int error_count = arg_parse(argc, argv, (void **)&set_args);
    if (error_count != 0) {
        arg_print_errors(stderr, set_args.end, argv[0]);
        return 1;
    }

    const char *key = set_args.key->sval[0];
    const char *val_str = set_args.val->sval[0];

    nvs_handle_t handle;  // zmienna na klucz dostepu do pamieci
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);

    if (err != ESP_OK) {
        printf("Blad otwarcia pamieci NVS\n");
        return 1;
    }

    if (strcmp(val_str, "false") == 0) {
        err = nvs_set_u8(handle, key, 0);
    }else if (strcmp(val_str, "true") == 0) {
        err = nvs_set_u8(handle, key, 1);
    }
    else {
        char *endptr;
        if (strchr(val_str, '.') != NULL) {
            float val_float = strtof(val_str, &endptr);
            if (*endptr == '\0') {
                err = nvs_set_blob(handle, key, &val_float, sizeof(float));
            }else {
                err = nvs_set_str(handle, key, val_str);
            }
        }else {
            // czy liczba calkowita
            long val_int = strtol(val_str, &endptr, 10);
            
            if (*endptr == '\0') {
                err = nvs_set_i32(handle, key, (int32_t)val_int);
            }else{
                err = nvs_set_str(handle, key, val_str);
            }
        }    
    }

    if (err == ESP_OK) {
        nvs_commit(handle);
    }

    nvs_close(handle);
    return 0;
}

static struct{
    struct arg_str *key;
    struct arg_end *end;
} del_args;

static int del_cmd(int argc, char **argv) {
    int error_count = arg_parse(argc, argv, (void **)&del_args);
    if (error_count != 0) {
        arg_print_errors(stderr, del_args.end, argv[0]);
        return 1;
    }

    const char *key = del_args.key->sval[0];
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);

    if (err != ESP_OK) {
        printf("Blad otwarcia pamieci NVS\n");
        return 1;
    }

    err = nvs_erase_key(handle, key);

    if (err == ESP_OK) {
        printf("Pomyslnie usunieto zmienna: %s\n", key);
    }else if (err == ESP_ERR_NVS_NOT_FOUND) {
        printf("Zmienna %s nie istnieje\n", key);
    }else {
        printf("Napotkano blad przy probie usuwania zmiennej %s\n", key);
        printf("%s\n", esp_err_to_name(err));
    }

    if (err == ESP_OK) {
        nvs_commit(handle);
    }

    nvs_close(handle);
    return 0;
}

void register_set_cmd(void) {
    set_args.key = arg_str1(NULL, NULL, "<nazwa>", "Nazwa zmiennej");
    set_args.val = arg_str1(NULL, NULL, "<wartosc>", "Wartosc zmiennej (bool, int lub tekst)");
    set_args.end = arg_end(2);

    const esp_console_cmd_t cmd = {
        .command = "set",
        .help = "Zapisuje lub modyfikuje zmienna konfiguracyjna",
        .hint = NULL,
        .func = &set_cmd,
        .argtable = &set_args
    };
    
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void register_del_cmd(void) {
    del_args.key = arg_str1(NULL, NULL, "<nazwa>", "Nazwa zmiennej do usuniecia");
    del_args.end = arg_end(1);

    const esp_console_cmd_t cmd = {
        .command = "del",
        .help = "Usuwa zmienna konfiguracyjna",
        .hint = NULL,
        .func = &del_cmd,
        .argtable = &del_args
    };
    
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}


void app_main(void){
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    
    repl_config.prompt = "space>";
    
    esp_console_register_help_command();
    register_set_cmd();
    register_del_cmd();


    ESP_ERROR_CHECK(esp_console_new_repl_stdio(&repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}