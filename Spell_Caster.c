#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>

#define API_URL "https://api.open5e.com/spells/"
#define HISTORY_FILE "spell_history.txt"

struct MemoryStruct {
    char* memory;
    size_t size;
};

size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct* mem = (struct MemoryStruct*)userp;

    char* ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

char* get_spell_info(const char* spell_name) {
    CURL* curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    // Bouw de URL op voor het verzoek aan de D&D 5e API
    char url[256];
    snprintf(url, sizeof(url), "%s%s/", API_URL, spell_name);

    // Initialiseer de libcurl bibliotheek
    curl_global_init(CURL_GLOBAL_ALL);

    // Initialiseer de curl handle
    curl_handle = curl_easy_init();

    // Stel de URL in
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    // Stel de schrijffunctie in voor de respons
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    // Stel de gebruikersgegevens in voor de schrijffunctie
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&chunk);

    // Voer het HTTP-verzoek uit
    res = curl_easy_perform(curl_handle);

    // Controleer op fouten
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(chunk.memory);
        curl_easy_cleanup(curl_handle);
        return NULL;
    }

    // Beëindig de curl handle
    curl_easy_cleanup(curl_handle);

    // Voeg een terminerende nul toe aan de ontvangen gegevens
    char* response = realloc(chunk.memory, chunk.size + 1);
    response[chunk.size] = '\0';

    return response;
}

void display_spell_card(const char* spell_data) {
    struct json_object* root;
    struct json_object* spell_name;
    struct json_object* spell_level;
    struct json_object* spell_school;
    struct json_object* spell_desc;

    // Parse de JSON-gegevens
    root = json_tokener_parse(spell_data);

    // Haal de waarde op voor elk veld
    json_object_object_get_ex(root, "name", &spell_name);
    json_object_object_get_ex(root, "level", &spell_level);
    json_object_object_get_ex(root, "school", &spell_school);
    json_object_object_get_ex(root, "desc", &spell_desc);

    // Toon de spell card met relevante informatie
    printf("---------------------------------\n");
    printf("Naam: %s\n", json_object_get_string(spell_name));
    printf("Niveau: %s\n", json_object_get_string(spell_level));
    printf("School: %s\n", json_object_get_string(spell_school));
    printf("Beschrijving: %s\n", json_object_get_string(spell_desc));
    printf("---------------------------------\n");

    // Vrijgeven van resources
    json_object_put(root);
}

void save_to_history(const char* spell_name) {
    FILE* file = fopen(HISTORY_FILE, "a");
    if (file == NULL) {
        fprintf(stderr, "Kan het geschiedenisbestand niet openen.\n");
        return;
    }

    fprintf(file, "%s\n", spell_name);
    fclose(file);

    printf("Keuze opgeslagen in het geschiedenisbestand.\n");
}

void display_history() {
    FILE* file = fopen(HISTORY_FILE, "r");
    if (file == NULL) {
        fprintf(stderr, "Kan het geschiedenisbestand niet openen.\n");
        return;
    }

    char line[256];
    printf("Geschiedenis van gemaakte keuzes:\n");
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;  // Verwijder newline-teken
        printf("- %s\n", line);
    }

    fclose(file);
}

int main() {
    char spell_name[256];

    printf("Voer de naam van de spreuk in: ");
    scanf("%s", spell_name);

    // Haal de informatie op over de ingevoerde spreuk
    char* spell_info = get_spell_info(spell_name);

    if (spell_info) {
        // Toon de spell card als de informatie beschikbaar is
        display_spell_card(spell_info);
        save_to_history(spell_name);
        free(spell_info);
    }

    display_history();

    return 0;
}
