## 📌 Overview
This repository contains the final project for the "Algorithms and Principles of Computer Science" course. The project simulates an industrial pastry shop's order management system, efficiently handling recipe management, ingredient inventory, and order fulfillment with periodic deliveries.

## 🚀 Features
- **📖 Recipe Management:** Add and remove recipes containing multiple ingredients.
- **📦 Inventory Handling:** Track ingredient stock with expiration dates and restocking.
- **🛒 Order Processing:** Accept, queue, or fulfill customer orders based on available stock.
- **🚚 Courier Logistics:** Manage scheduled pickups, prioritizing orders by weight and arrival time.

## 🛠 Installation & Usage
### Prerequisites
- A C compiler (e.g., GCC)
- Standard C libraries

### 🏗 Build
Clone the repository and compile the project:
```sh
git clone https://github.com/MattiaBrianti/PFAPI24_BRIANTI_10773859.git
cd PFAPI24_BRIANTI_10773859
gcc -o pastry_shop main.c
```

### ▶ Run the Program
Execute the program with an input file:
```sh
./pastry_shop < input.txt
```
where `input.txt` contains a sequence of commands following the project specifications.

## 📝 Command Format
The program processes commands in the following format:
- `aggiungi_ricetta <recipe_name> <ingredient_1> <quantity_1> ...`
- `rimuovi_ricetta <recipe_name>`
- `rifornimento <ingredient> <quantity> <expiration>`
- `ordine <recipe_name> <quantity>`

## 📊 Expected Output
The program provides real-time feedback:
- `aggiunta` or `ignorato` for recipe additions
- `rimossa`, `ordini in sospeso`, or `non presente` for recipe removals
- `rifornito` for restocking
- `accettato` or `rifiutato` for orders
- Periodic courier shipment logs listing dispatched orders in priority order

## ⚡ Performance
<div align="center">

| **Memory Usage**       | **Execution Time** |
|:----------------------:|:-----------------:|
| 25.0 MiB              | ~13.138 s         |

*Performance tested internally.*

</div>


---

_This repository is part of the final project for the "Algorithms and Principles of Computer Science" course._

