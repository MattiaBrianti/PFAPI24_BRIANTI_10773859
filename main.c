#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LENGTH 20
#define HASHMAP_CAPACITY 32771

typedef struct ingredient {
    char *name;
    size_t ing_index;
    int32_t quantity;
    struct ingredient *next;
} Ingredient;

typedef struct lotto {
    int32_t ingredient_quantity;
    int32_t ingredient_expiration_date;
} Lotto;

typedef struct recipe {
    Ingredient *ingredients;
} Recipe;

typedef struct Entry {
    char *key;
    void *value;
    struct Entry *next;
} Entry;

typedef struct HashMap {
    Entry **table;
} HashMap;
HashMap *catalog_tree = NULL;
HashMap *warehouse_tree = NULL;

typedef struct EntryInt {
    char *key;
    int value;
    struct EntryInt *next;
} EntryInt;

typedef struct HashMapInt {
    EntryInt **table;
} HashMapInt;

typedef struct order {
    char *recipe_name;
    size_t rec_index;
    int32_t quantity;
    int32_t order_timestamp;
    struct order *next;
} Order;
Order *orders_ready_queue = NULL;
Order *orders_wait_queue = NULL;
Order *orders_wait_queue_tail = NULL;
Order *shipment_queue = NULL;

typedef struct carrier {
    int32_t periodicity;
    int32_t capacity;
} Carrier;

int32_t current_timestamp = 0;

size_t hash(char *key);
HashMap *hashmap_create();
HashMapInt *hashmap_int_create();
void hashmap_put_recipes(HashMap *map, char *key, void *value, size_t);
void hashmap_int_put_recipes(HashMapInt *map, char *key, int value, size_t);
void hashmap_put_ordered(HashMap *map, char *key, void *value, size_t);
void *hashmap_get_recipe(HashMap *map, char *key, size_t);
int hashmap_int_get_recipe(HashMapInt *map, char *key, size_t);
void *hashmap_get_lotto(HashMap *map, char *key, int, size_t);
Entry *hashmap_get_entries(HashMap *map, char *key, size_t);
void hashmap_remove_recipe(HashMap *map, char *key, size_t);
void hashmap_remove_entry(HashMap *map, char *key, Entry *entry_to_remove,
                          size_t);
void hashmap_free(HashMap *map);
void hashmap_int_free(HashMapInt *map);
Ingredient *create_ingredient(char *, int32_t);
void add_ingredient_to_recipe(Recipe *, Ingredient *);
Lotto *create_lotto(int32_t, int32_t);
Order *create_order(char *, int32_t, size_t);
Order *create_order_timestamp(char *, int32_t, int32_t, size_t);
void analyze_order(Order *, Recipe *);
int evaluate_shifting_order(char *, int32_t, int32_t, size_t, Recipe *);
void add_order_to_ready_queue(Order *);
void add_order_to_wait_queue(Order *);
void add_order_to_shipment_queue(Order *, Order *);
int get_order_heaviness(Order *);
Carrier *create_carrier(int32_t, int32_t);
void destroy_ingredient(Ingredient *);
void destroy_order(Order **);
Carrier *manage_carrier();
void manage_ingredients(Recipe *, char *);
int seek_recipe_in_wait_list(char *);
int seek_recipe_in_ready_list(char *);
void print_carrier_content(int32_t);
void shift_orders_from_wait_to_ready_queue();

char *read_word(int *new_line) {
    static char buffer[MAX_LENGTH];
    int i = 0;
    int c;

    // Skip leading whitespace
    while ((c = getc_unlocked(stdin)) != EOF && isspace(c) && c != '\n')
        ;

    if (c == EOF)
        return NULL;

    // Read the word
    do {
        if (i < MAX_LENGTH - 1)
            buffer[i++] = c;
    } while ((c = getc_unlocked(stdin)) != EOF && !isspace(c) && c != '\n');

    buffer[i] = '\0';
    // If we stopped because of a newline returns 1
    if (c == '\n')
        *new_line = 1;
    else
        *new_line = 0;
    return buffer;
}

int read_int(int *new_line) {
    int number = 0;
    int c;

    // Skip leading whitespace
    while ((c = getc_unlocked(stdin)) != EOF && isspace(c) && c != '\n')
        ;

    if (c == EOF)
        return 0;

    while (c != EOF && isdigit(c) && c != '\n') {
        number = number * 10 + (c - '0');
        c = getc_unlocked(stdin);
    }
    // If we stopped because of a newline returns 1
    if (c == '\n')
        *new_line = 1;
    else
        *new_line = 0;
    return number;
}

void print(HashMap *map) {
    for (int i = 0; i < HASHMAP_CAPACITY; i++) {
        Entry *entry = map->table[i];
        while (entry != NULL) {
            Entry *next = entry->next;
            printf("%s[%d]: qty:%d, exp:%d, ", entry->key, i,
                   ((Lotto *)entry->value)->ingredient_quantity,
                   ((Lotto *)entry->value)->ingredient_expiration_date);
            entry = next;
        }
        if (entry != NULL)
            printf("\n");
    }
}

void print_queue(Order *iter) {
    while (iter != NULL) {
        printf("%s, ts: %d, qty: %d\n", iter->recipe_name, iter->order_timestamp,
               iter->quantity);

        iter = iter->next;
    }
}

int main() {
    char *input, *param;
    int new_line = 0;

    catalog_tree = hashmap_create();
    warehouse_tree = hashmap_create();

    // reading <periodicity, capacity> of the carrier
    Carrier *carrier = manage_carrier();

    while ((input = read_word(&new_line)) != NULL) {
        if (current_timestamp % carrier->periodicity == 0 && current_timestamp != 0)
            print_carrier_content(carrier->capacity);

        if (strcmp(input, "aggiungi_ricetta") == 0) {
            if ((param = read_word(&new_line)) != NULL) {
                size_t rec_index = hash(param);
                Recipe *rec =
                        (Recipe *)hashmap_get_recipe(catalog_tree, param, rec_index);
                if (rec != NULL) {
                    printf("ignorato\n");
                    fflush(stdout);

                    // Skip the rest of the line
                    while (new_line == 0 && (param = read_word(&new_line))) {
                        read_int(&new_line);
                    }
                } else {
                    // reading recipe name, create and insert recipe into the catalog
                    Recipe *recipe = (Recipe *)malloc(sizeof(Recipe));
                    recipe->ingredients = NULL;
                    hashmap_put_recipes(catalog_tree, param, recipe, rec_index);
                    printf("aggiunta\n");
                    fflush(stdout);

                    // reading all the ingredients pairs<ingredient_name, quantity> adding
                    // them to the related recipe
                    manage_ingredients(recipe, param);
                }
            }
        } else if (strcmp(input, "rimuovi_ricetta") == 0) {
            if ((param = read_word(&new_line)) != NULL) {
                size_t rec_index = hash(param);
                Recipe *recipe =
                        (Recipe *)hashmap_get_recipe(catalog_tree, param, rec_index);
                if (recipe == NULL) {
                    printf("non presente\n");
                    fflush(stdout);
                } else if (seek_recipe_in_wait_list(param) ||
                           seek_recipe_in_ready_list(param)) {
                    printf("ordini in sospeso\n");
                    fflush(stdout);
                } else {
                    hashmap_remove_recipe(catalog_tree, param, rec_index);
                    printf("rimossa\n");
                    fflush(stdout);
                }
            }
        } else if (strcmp(input, "rifornimento") == 0) {
            int ingredient_quantity = 0, ingredient_expiration_date = 0;
            while (new_line == 0 && (param = read_word(&new_line))) {
                size_t lot_index = hash(param);
                ingredient_quantity = read_int(&new_line);
                ingredient_expiration_date = read_int(&new_line);
                Lotto *lot = (Lotto *)hashmap_get_lotto(
                        warehouse_tree, param, ingredient_expiration_date, lot_index);
                if (lot != NULL)
                    lot->ingredient_quantity += ingredient_quantity;
                else {
                    if (ingredient_expiration_date > current_timestamp) {
                        hashmap_put_ordered(
                                warehouse_tree, param,
                                create_lotto(ingredient_quantity, ingredient_expiration_date),
                                lot_index);
                    }
                }
            }
            shift_orders_from_wait_to_ready_queue();
            printf("rifornito\n");
            fflush(stdout);
        } else if (strcmp(input, "ordine") == 0) {
            if ((param = read_word(&new_line)) != NULL) {
                int order_quantity = read_int(&new_line);
                size_t rec_index = hash(param);
                Recipe *rec = hashmap_get_recipe(catalog_tree, param, rec_index);
                if (rec != NULL) {
                    printf("accettato\n");
                    fflush(stdout);
                    analyze_order(create_order(param, order_quantity, rec_index), rec);
                } else {
                    printf("rifiutato\n");
                    fflush(stdout);
                }
            }
        }
        current_timestamp++;
    }

    if (current_timestamp % carrier->periodicity == 0 && current_timestamp != 0)
        print_carrier_content(carrier->capacity);

    hashmap_free(catalog_tree);
    hashmap_free(warehouse_tree);
    free(carrier);
    carrier = NULL;
    destroy_order(&orders_wait_queue);
    destroy_order(&orders_ready_queue);
    destroy_order(&shipment_queue);

    return 0;
}

size_t hash(char *key) {
    size_t hash = 2166136261;
    size_t prime = 16777219;

    while (*key != '\0') {
        hash ^= (size_t)(*key);
        hash *= prime;
        key++;
    }

    return hash % HASHMAP_CAPACITY;
}

HashMap *hashmap_create() {
    HashMap *map = malloc(sizeof(HashMap));
    map->table = calloc(HASHMAP_CAPACITY, sizeof(Entry *));
    return map;
}

HashMapInt *hashmap_int_create() {
    HashMapInt *map = malloc(sizeof(HashMapInt));
    map->table = calloc(HASHMAP_CAPACITY, sizeof(EntryInt *));
    return map;
}

// LIFO adjacent queue management
void hashmap_put_recipes(HashMap *map, char *key, void *value, size_t index) {
    Entry *entry = map->table[index];
    Entry *prev = NULL;

    while (entry != NULL && strcmp(entry->key, key) <= 0) {
        if (entry->next != NULL && strcmp(entry->key, key) == 0 &&
            strcmp(entry->next->key, key) != 0)
            break;
        prev = entry;
        entry = entry->next;
    }
    // Create new entry
    Entry *new_entry = malloc(sizeof(Entry));
    new_entry->key = strdup(key);
    new_entry->value = value;
    if (prev != NULL) {
        prev->next = new_entry;
    } else {
        map->table[index] = new_entry;
    }
    new_entry->next = entry;
}

// LIFO adjacent queue management
void hashmap_int_put_recipes(HashMapInt *map, char *key, int value,
                             size_t index) {
    EntryInt *entry = map->table[index];
    EntryInt *prev = NULL;

    while (entry != NULL && strcmp(entry->key, key) <= 0) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;
            return;
        }
        prev = entry;
        entry = entry->next;
    }
    // Create new entry
    EntryInt *new_entry = malloc(sizeof(EntryInt));
    new_entry->key = strdup(key);
    new_entry->value = value;
    if (prev != NULL) {
        prev->next = new_entry;
    } else {
        map->table[index] = new_entry;
    }
    new_entry->next = entry;
}

// Alphabetical and Expiration data ordered adjacent queue management
void hashmap_put_ordered(HashMap *map, char *key, void *value, size_t index) {
    Entry *entry = map->table[index];
    Entry *prev = NULL;

    while (entry != NULL && strcmp(entry->key, key) <= 0) {
        if (current_timestamp >=
            ((Lotto *)entry->value)->ingredient_expiration_date) {
            // Expired lotto
            Entry *next = entry->next;

            if (prev == NULL) {
                map->table[index] = next;
                prev = NULL;
            } else {
                prev->next = next;
            }
            free(entry->key);
            free(entry);
            entry = next;
        } else {
            if (entry->next != NULL) {
                if (strcmp(entry->key, key) < 0) {
                    prev = entry;
                    entry = entry->next;
                } else if (strcmp(entry->key, key) == 0) {
                    if (strcmp(entry->next->key, key) != 0) {
                        if (((Lotto *)entry->value)->ingredient_expiration_date >=
                            ((Lotto *)value)->ingredient_expiration_date)
                            break;
                        else {
                            prev = entry;
                            entry = entry->next;
                            break;
                        }
                    } else { // strcmp(entry->next->key, key) == 0
                        if (((Lotto *)entry->value)->ingredient_expiration_date <
                            ((Lotto *)value)->ingredient_expiration_date) {
                            prev = entry;
                            entry = entry->next;
                        } else
                            break;
                    }
                } else { // strcmp(entry->key, key) > 0
                    exit(1);
                }
            }
            if (entry->next == NULL) {
                if (strcmp(entry->key, key) > 0) {
                    break;
                } else if (strcmp(entry->key, key) == 0) {
                    if (((Lotto *)entry->value)->ingredient_expiration_date >=
                        ((Lotto *)value)->ingredient_expiration_date) {
                        break;
                    } else {
                        prev = entry;
                        entry = entry->next;
                        break;
                    }
                } else {
                    prev = entry;
                    entry = entry->next;
                    break;
                }
            }
        }
    }
    // Create new entry
    Entry *new_entry = malloc(sizeof(Entry));
    new_entry->key = strdup(key);
    new_entry->value = value;
    if (prev != NULL) {
        prev->next = new_entry;
    } else {
        map->table[index] = new_entry;
    }
    new_entry->next = entry;
}

void *hashmap_get_recipe(HashMap *map, char *key, size_t index) {
    Entry *entry = map->table[index];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        if (strcmp(entry->key, key) < 0) {
            entry = entry->next;
        } else
            break;
    }
    return NULL; // Key not found
}

int hashmap_int_get_recipe(HashMapInt *map, char *key, size_t index) {
    EntryInt *entry = map->table[index];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        if (strcmp(entry->key, key) < 0) {
            entry = entry->next;
        } else
            break;
    }
    return -1; // Key not found
}

void *hashmap_get_lotto(HashMap *map, char *key, int expiration_date,
                        size_t index) {
    Entry *prev = NULL;
    Entry *entry = map->table[index];

    while (entry != NULL) {
        if (current_timestamp >=
            ((Lotto *)entry->value)->ingredient_expiration_date) {
            // Expired lotto
            Entry *next = entry->next;
            if (prev == NULL) {
                map->table[index] = next;
                prev = NULL;
            } else {
                prev->next = next;
            }

            free(entry->key);
            free(entry);

            entry = next;
        } else {
            if (strcmp(entry->key, key) == 0 &&
                ((Lotto *)entry->value)->ingredient_expiration_date ==
                expiration_date) {
                return entry->value;
            }

            prev = entry;
            entry = entry->next;
        }
    }
    return NULL; // Key not found
}

Entry *hashmap_get_entries(HashMap *map, char *key, size_t index) {
    Entry *entry = map->table[index];

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL; // Key not found
}

Entry *hashmap_get_and_remove_expired_entries(HashMap *map, char *key,
                                              size_t index) {
    Entry *entry = map->table[index];
    Entry *prev = NULL;

    while (entry != NULL) {
        if (current_timestamp >=
            ((Lotto *)entry->value)->ingredient_expiration_date) {
            // Expired lotto
            Entry *next = entry->next;
            if (prev == NULL) {
                map->table[index] = next;
                prev = NULL;
            } else {
                prev->next = next;
            }

            free(entry->key);
            free(entry);
            entry = next;
        } else {
            if (strcmp(entry->key, key) == 0) {
                return entry;
            }
            prev = entry;
            entry = entry->next;
        }
    }
    return NULL; // Key not found
}

void hashmap_remove_recipe(HashMap *map, char *key, size_t index) {
    Entry *prev = NULL;
    Entry *entry = map->table[index];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            // Found the entry to remove
            if (prev == NULL) {
                map->table[index] = entry->next;
            } else {
                prev->next = entry->next;
            }
            free(entry->key);
            destroy_ingredient(((Recipe *)entry->value)->ingredients);
            free(entry);
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

void hashmap_remove_entry(HashMap *map, char *key, Entry *entry_to_remove,
                          size_t index) {
    Entry *prev = NULL;
    Entry *entry = map->table[index];

    while (entry->next != NULL && entry != entry_to_remove) {
        prev = entry;
        entry = entry->next;
    }

    if (prev == NULL) {
        map->table[index] = entry->next;
    } else {
        prev->next = entry->next;
    }
    free(entry->key);
    free(entry);
}

void hashmap_free(HashMap *map) {
    for (int i = 0; i < HASHMAP_CAPACITY; i++) {
        Entry *entry = map->table[i];
        while (entry != NULL) {
            Entry *next = entry->next;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    free(map->table);
    free(map);
}

void hashmap_int_free(HashMapInt *map) {
    for (int i = 0; i < HASHMAP_CAPACITY; i++) {
        EntryInt *entry = map->table[i];
        while (entry != NULL) {
            EntryInt *next = entry->next;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    free(map->table);
    free(map);
}

Ingredient *create_ingredient(char *name, int32_t quantity) {
    Ingredient *ing = (Ingredient *)malloc(sizeof(Ingredient));
    ing->name = malloc(MAX_LENGTH * sizeof(char));
    strcpy(ing->name, name);
    ing->ing_index = hash(name);
    ing->quantity = quantity;
    ing->next = NULL;
    return ing;
}

// adding ingredient in a LIFO queue
void add_ingredient_to_recipe(Recipe *recipe, Ingredient *ingredient) {
    if (recipe->ingredients == NULL)
        recipe->ingredients = ingredient;
    else {
        ingredient->next = recipe->ingredients;
        recipe->ingredients = ingredient;
    }
}

Lotto *create_lotto(int32_t ingredient_quantity,
                    int32_t ingredient_expiration_date) {
    Lotto *lot = (Lotto *)malloc(sizeof(Lotto));
    lot->ingredient_quantity = ingredient_quantity;
    lot->ingredient_expiration_date = ingredient_expiration_date;
    return lot;
}

Order *create_order(char *recipe_name, int32_t quantity, size_t rec_index) {
    Order *ord = (Order *)malloc(sizeof(Order));
    ord->recipe_name = (char *)malloc(MAX_LENGTH * sizeof(char));
    strcpy(ord->recipe_name, recipe_name);
    ord->rec_index = rec_index;
    ord->quantity = quantity;
    ord->order_timestamp = current_timestamp;
    ord->next = NULL;
    return ord;
}

Order *create_order_timestamp(char *recipe_name, int32_t quantity,
                              int32_t timestamp, size_t rec_index) {
    Order *ord = (Order *)malloc(sizeof(Order));
    ord->recipe_name = (char *)malloc(MAX_LENGTH * sizeof(char));
    strcpy(ord->recipe_name, recipe_name);
    ord->rec_index = rec_index;
    ord->quantity = quantity;
    ord->order_timestamp = timestamp;
    ord->next = NULL;
    return ord;
}

void analyze_order(Order *order, Recipe *recipe) {
    int is_ready = 1;
    Ingredient *ingredient = recipe->ingredients;

    if (ingredient == NULL)
        exit(1);

    while (ingredient != NULL && is_ready) { // check availability and decide if
        // order is ready or in wait state
        int32_t ingredient_total_quantity = ingredient->quantity * order->quantity;
        Entry *entries = hashmap_get_and_remove_expired_entries(
                warehouse_tree, ingredient->name, ingredient->ing_index);

        if (entries == NULL) {
            is_ready = 0;
            add_order_to_wait_queue(order);
        } else {
            while (entries != NULL) { //&& strcmp(entries->key, ingredient->name)<=0
                if (strcmp(entries->key, ingredient->name) == 0) {
                    if (((Lotto *)entries->value)->ingredient_quantity >=
                        ingredient_total_quantity) {
                        ingredient_total_quantity = 0;
                        break;
                    } else
                        ingredient_total_quantity -=
                                ((Lotto *)entries->value)->ingredient_quantity;
                }
                entries = entries->next;
            }
            if (ingredient_total_quantity > 0) {
                is_ready = 0;
                add_order_to_wait_queue(order);
            }
            ingredient = ingredient->next;
        }
    }

    if (is_ready) { // updating warehouse stocks for each ingredient of the order
        // and eventually add it to the ready queue
        ingredient = recipe->ingredients;
        if (ingredient == NULL)
            exit(1);
        while (ingredient != NULL) {

            int32_t ingredient_total_qty = ingredient->quantity * order->quantity;
            while (ingredient_total_qty > 0) {
                Entry *entries = hashmap_get_entries(warehouse_tree, ingredient->name,
                                                     ingredient->ing_index);
                Entry *min = entries;
                while (entries != NULL) {
                    if (strcmp(entries->key, ingredient->name) == 0) {
                        min = entries;
                        break;
                    }
                    entries = entries->next;
                }

                if (((Lotto *)min->value)->ingredient_quantity >=
                    ingredient_total_qty) {
                    ((Lotto *)min->value)->ingredient_quantity -= ingredient_total_qty;
                    ingredient_total_qty = 0;
                } else {
                    ingredient_total_qty -= ((Lotto *)min->value)->ingredient_quantity;
                    ((Lotto *)min->value)->ingredient_quantity = 0;
                }
                if (((Lotto *)min->value)->ingredient_quantity == 0)
                    hashmap_remove_entry(warehouse_tree, ingredient->name, min,
                                         ingredient->ing_index);
            }
            ingredient = ingredient->next;
        }
        add_order_to_ready_queue(order);
    }
}

int evaluate_shifting_order(char *order_name, int32_t order_qty,
                            int32_t order_timestamp, size_t rec_index,
                            Recipe *recipe) {
    int is_ready = 1;
    Ingredient *ingredient = recipe->ingredients;

    if (ingredient == NULL)
        exit(1);

    while (ingredient != NULL && is_ready) { // check availability and decide if
        // order is ready or in wait state
        int32_t ingredient_total_quantity = ingredient->quantity * order_qty;

        Entry *entries = hashmap_get_and_remove_expired_entries(
                warehouse_tree, ingredient->name, ingredient->ing_index);
        if (entries == NULL) {
            is_ready = 0;
        } else {
            while (entries != NULL) {
                if (strcmp(entries->key, ingredient->name) == 0) {
                    if (((Lotto *)entries->value)->ingredient_quantity >=
                        ingredient_total_quantity) {
                        ingredient_total_quantity = 0;
                        break;
                    } else
                        ingredient_total_quantity -=
                                ((Lotto *)entries->value)->ingredient_quantity;
                }
                entries = entries->next;
            }
            if (ingredient_total_quantity > 0)
                is_ready = 0;
            ingredient = ingredient->next;
        }
    }

    if (is_ready) { // updating warehouse stocks for each ingredient of the order
        // and eventually add it to the ready queue
        ingredient = recipe->ingredients;
        if (ingredient == NULL)
            exit(1);
        while (ingredient != NULL) {
            int32_t ingredient_total_qty = ingredient->quantity * order_qty;

            while (ingredient_total_qty > 0) {
                Entry *entries = hashmap_get_entries(warehouse_tree, ingredient->name,
                                                     ingredient->ing_index);
                Entry *min = entries;
                while (entries != NULL) {
                    if (strcmp(entries->key, ingredient->name) == 0) {
                        min = entries;
                        break;
                    }
                    entries = entries->next;
                }
                if (((Lotto *)min->value)->ingredient_quantity >=
                    ingredient_total_qty) {
                    ((Lotto *)min->value)->ingredient_quantity -= ingredient_total_qty;
                    ingredient_total_qty = 0;
                } else {
                    ingredient_total_qty -= ((Lotto *)min->value)->ingredient_quantity;
                    ((Lotto *)min->value)->ingredient_quantity = 0;
                }
                if (((Lotto *)min->value)->ingredient_quantity == 0)
                    hashmap_remove_entry(warehouse_tree, ingredient->name, min,
                                         ingredient->ing_index);
            }
            ingredient = ingredient->next;
        }
        add_order_to_ready_queue(create_order_timestamp(
                order_name, order_qty, order_timestamp, rec_index));
    }
    return is_ready;
}

void add_order_to_wait_queue(Order *order) {
    if (orders_wait_queue == NULL) {
        orders_wait_queue = order;
    } else {
        orders_wait_queue_tail->next = order;
    }

    orders_wait_queue_tail = order;
    order->next = NULL;
}

void add_order_to_ready_queue(Order *order) {
    // adding orders in a timestamp-ordered queue
    if (orders_ready_queue == NULL) {
        orders_ready_queue = order;
        return;
    }

    Order *iterator = orders_ready_queue;
    Order *step_before_iterator = orders_ready_queue;

    while (iterator != NULL &&
           order->order_timestamp > iterator->order_timestamp) {
        step_before_iterator = iterator;
        iterator = iterator->next;
    }

    if (step_before_iterator != iterator) { // push_back
        step_before_iterator->next = order;
        order->next = iterator;
    } else { // push_front
        Order *aus = orders_ready_queue;
        orders_ready_queue = order;
        order->next = aus;
    }
}

void add_order_to_shipment_queue(Order *order, Order *ready_queue_order) {
    if (order != ready_queue_order) {
        if (shipment_queue == NULL) { // empty queue
            shipment_queue = order;
            orders_ready_queue = orders_ready_queue->next;
            order->next = NULL;
        } else { // push_back
            Order *shipment_iterator = shipment_queue;
            Order *step_before_shipment_iterator = shipment_queue;

            while (shipment_iterator != NULL &&
                   get_order_heaviness(order) <=
                   get_order_heaviness(shipment_iterator)) {
                if (get_order_heaviness(order) ==
                    get_order_heaviness(shipment_iterator)) {
                    while (shipment_iterator != NULL &&
                           get_order_heaviness(order) ==
                           get_order_heaviness(shipment_iterator) &&
                           order->order_timestamp > shipment_iterator->order_timestamp) {
                        step_before_shipment_iterator = shipment_iterator;
                        shipment_iterator = shipment_iterator->next;
                    }
                    break;
                }
                step_before_shipment_iterator = shipment_iterator;
                shipment_iterator = shipment_iterator->next;
            }

            if (step_before_shipment_iterator != shipment_iterator) {
                step_before_shipment_iterator->next = order;
                orders_ready_queue = orders_ready_queue->next;
                order->next = shipment_iterator;
            } else {
                Order *aus = shipment_queue;
                shipment_queue = order;
                orders_ready_queue = orders_ready_queue->next;
                order->next = aus;
            }
        }
    }
}

int get_order_heaviness(Order *order) {
    int32_t total_heaviness = 0;
    Ingredient *iterator = NULL;
    Recipe *rec =
            hashmap_get_recipe(catalog_tree, order->recipe_name, order->rec_index);

    if (rec != NULL)
        iterator = rec->ingredients;

    while (iterator != NULL) {
        total_heaviness += order->quantity * iterator->quantity;
        iterator = iterator->next;
    }

    return total_heaviness;
}

Carrier *create_carrier(int32_t periodicity, int32_t capacity) {
    Carrier *car = (Carrier *)malloc(sizeof(Carrier));
    car->capacity = capacity;
    car->periodicity = periodicity;
    return car;
}

void destroy_ingredient(Ingredient *ingredient) {
    if (ingredient == NULL)
        return;
    free(ingredient->name);
    ingredient->name = NULL;
    destroy_ingredient(ingredient->next);
    ingredient->next = NULL;
    free(ingredient);
    ingredient = NULL;
}

void destroy_order(Order **order) {
    if (order == NULL || *order == NULL)
        return;
    free((*order)->recipe_name);
    (*order)->recipe_name = NULL;
    destroy_order(&((*order)->next));
    (*order)->next = NULL;
    free(*order);
    *order = NULL;
    order = NULL;
}

Carrier *manage_carrier() {
    int new_line = 0;
    int periodicity = read_int(&new_line);
    int capacity = read_int(&new_line);
    Carrier *carrier = create_carrier(periodicity, capacity);
    return carrier;
}

void manage_ingredients(Recipe *recipe, char *ingredient_name) {
    int new_line = 0;
    while (new_line == 0 && (ingredient_name = read_word(&new_line))) {
        int ingredient_quantity = read_int(&new_line);
        add_ingredient_to_recipe(
                recipe, create_ingredient(ingredient_name, ingredient_quantity));
    }
}

int seek_recipe_in_wait_list(char *recipe_name) {
    int result = 0;
    Order *iterator = orders_wait_queue;

    while (iterator != NULL && !result) {
        if (strcmp(iterator->recipe_name, recipe_name) == 0)
            result = 1;
        else
            iterator = iterator->next;
    }

    return result;
}

int seek_recipe_in_ready_list(char *recipe_name) {
    int result = 0;
    Order *iterator = orders_ready_queue;

    while (iterator != NULL && !result) {
        if (strcmp(iterator->recipe_name, recipe_name) == 0)
            result = 1;
        else
            iterator = iterator->next;
    }

    return result;
}

void print_carrier_content(int32_t carrier_capacity) {
    int32_t current_weight = 0;
    Order *step_before_iterator = orders_ready_queue;
    Order *iterator = orders_ready_queue;

    if (iterator == NULL) {
        printf("camioncino vuoto\n");
        fflush(stdout);
        return;
    }

    while (iterator != NULL) {
        if (current_weight + get_order_heaviness(iterator) <= carrier_capacity) {
            current_weight += get_order_heaviness(iterator);

            add_order_to_shipment_queue(step_before_iterator, iterator);

            step_before_iterator = iterator;
            iterator = iterator->next;
        } else
            break;
    }
    add_order_to_shipment_queue(step_before_iterator, iterator);

    Order *shipment_iterator = shipment_queue;
    Order *order = NULL;
    while (shipment_iterator != NULL) {

        printf("%d %s %d\n", shipment_iterator->order_timestamp,
               shipment_iterator->recipe_name, shipment_iterator->quantity);
        fflush(stdout);

        order = shipment_iterator;
        shipment_iterator = shipment_iterator->next;

        if (order == NULL)
            exit(1);
        free(order->recipe_name);
        order->recipe_name = NULL;
        order->next = NULL;
        free(order);
        order = NULL;
    }
    shipment_queue = NULL;
}

void shift_orders_from_wait_to_ready_queue() {
    Order *wait_order = orders_wait_queue;
    Order *prec_wait_order = NULL;
    HashMapInt *wait_map = hashmap_int_create();

    while (wait_order != NULL) {
        int wait_map_entry = hashmap_int_get_recipe(
                wait_map, wait_order->recipe_name, wait_order->rec_index);

        if (wait_map_entry != -1) {
            if (wait_order->quantity >= wait_map_entry) {
                prec_wait_order = wait_order;
                wait_order = wait_order->next;
                continue;
            }
        }

        Recipe *rec = hashmap_get_recipe(catalog_tree, wait_order->recipe_name,
                                         wait_order->rec_index);
        if (rec != NULL) {
            if (evaluate_shifting_order(wait_order->recipe_name, wait_order->quantity,
                                        wait_order->order_timestamp,
                                        wait_order->rec_index, rec)) {
                Order *aus = wait_order;

                if (prec_wait_order == NULL) {
                    orders_wait_queue = orders_wait_queue->next;
                    prec_wait_order = NULL;
                    wait_order = wait_order->next;
                } else {
                    prec_wait_order->next = wait_order->next;
                    wait_order = wait_order->next;
                }

                free(aus->recipe_name);
                aus->recipe_name = NULL;
                aus->next = NULL;
                free(aus);
                aus = NULL;
            } else {
                hashmap_int_put_recipes(wait_map, wait_order->recipe_name,
                                        wait_order->quantity, wait_order->rec_index);

                prec_wait_order = wait_order;
                wait_order = wait_order->next;
            }
        } else {
            prec_wait_order = wait_order;
            wait_order = wait_order->next;
            exit(1);
        }
    }

    if (wait_order == NULL) {
        orders_wait_queue_tail = prec_wait_order;
    }

    hashmap_int_free(wait_map);
}
