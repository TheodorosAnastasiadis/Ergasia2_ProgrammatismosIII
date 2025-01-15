#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>


typedef struct {
    char description[50];    // Περιγραφή του προϊόντος
    float price;           // Τιμή του προϊόντος
    int item_count;        // Πλήθος τεμαχίων του προϊόντος
} Product;


int total_orders = 0;         // Συνολικός αριθμός παραγγελιών
int successful_orders = 0;    // Συνολικός αριθμός επιτυχημένων παραγγελιών
int failed_orders = 0;        // Συνολικός αριθμός αποτυχημένων παραγγελιών
float total_revenue = 0.0;    // Συνολικός τζίρος


// Συνάρτηση για να αρχικοποιήσει τον κατάλογο των προϊόντων
void initialize_catalog(Product catalog[]) {
    for (int i = 0; i < 20; i++) {       // Διατρέχουμε τον κατάλογο για 20 προϊόντα
        sprintf(catalog[i].description, "Product %d", i);       // Ορίζουμε την περιγραφή του προϊόντος
        catalog[i].price = (float)(10 + i * 5); // Ορίζουμε την τιμή του προϊόντος (Τυχαία τιμή, ξεκινά από 10 και αυξάνεται κατά 5)
        catalog[i].item_count = 2;                // Ορίζουμε το πλήθος των τεμαχίων για κάθε προϊόν (2 τεμάχια ανά προϊόν)
    }
}

int main() {
    Product catalog[20];
    initialize_catalog(catalog);  // Αρχικοποίηση του καταλόγου προϊόντων

    int to_server[5][2], from_server[5][2];   // Δύο πίνακες pipes για επικοινωνία με τον server (5 διαδικασίες)
    pid_t pids[5];   // Πίνακας για τα PID των διαδικασιών

    // Δημιουργία των pipes
    for (int i = 0; i < 5; i++) {   // Δημιουργία των pipes για επικοινωνία μεταξύ της κύριας διαδικασίας και των υποδιεργασιών
        if (pipe(to_server[i]) == -1 || pipe(from_server[i]) == -1) {  // Δημιουργία pipe για επικοινωνία από την κύρια διαδικασία προς τον server
            perror("Pipe creation failed");  // Αν αποτύχει η δημιουργία pipe, εκτυπώνουμε σφάλμα
            exit(1);     // Έξοδος από το πρόγραμμα αν υπάρχει σφάλμα
        }
    }

    // Δημιουργία των pipes για επικοινωνία μεταξύ της κύριας διαδικασίας και των υποδιεργασιών
    for (int i = 0; i < 5; i++) {
        pids[i] = fork();     // Δημιουργεί μία νέα διεργασία και αποθηκεύει το PID στην αντίστοιχη θέση του πίνακα pids

        if (pids[i] == -1) {      // Ελέγχει αν η συνάρτηση fork απέτυχε (επιστρέφει -1 σε περίπτωση αποτυχίας)
            perror("Fork failed");   // Εμφάνιση μηνύματος λάθους
            exit(1);            // Έξοδος από το πρόγραμμα με σφάλμα
        }

        // Κώδικας που εκτελείται από τη διεργασία-πελάτη
        if (pids[i] == 0) {
            close(to_server[i][0]);    // Κλείσιμο του σωλήνα ανάγνωσης προς το κατάστημα (server)
            close(from_server[i][1]);  // Κλείσιμο του σωλήνα γραφής από το κατάστημα (server)

            srand(time(NULL) + i);   // Αρχικοποίηση τυχαίων αριθμών για τον συγκεκριμένο πελάτη

            for (int j = 0; j < 10; j++) {  // Επανάληψη για 10 αιτήματα από τον πελάτη
                int product_id = rand() % 20;   // Επιλογή τυχαίου ID προϊόντος (0-19)

                write(to_server[i][1], &product_id, sizeof(product_id));    // Αποστολή του product_id στο κατάστημα μέσω του σωλήνα

                char response[100];           // Ανάγνωση απάντησης από το κατάστημα
                read(from_server[i][0], response, sizeof(response));
                printf("Client %d: %s\n", i, response);     // Εμφάνιση της απάντησης στην οθόνη

                sleep(1);    // Αναμονή για 1 δευτερόλεπτο πριν το επόμενο αίτημα
            }

            close(to_server[i][1]);        // Κλείσιμο του σωλήνα γραφής προς το κατάστημα
            close(from_server[i][0]);      // Κλείσιμο του σωλήνα ανάγνωσης από το κατάστημα
            exit(0);                       // Τερματισμός της διεργασίας-πελάτη
        }
    }

    // Πατρική διεργασία (eshop)
    for (int i = 0; i < 5; i++) {
        close(to_server[i][1]);      // Κλείσιμο του pipe γραφής προς τους πελάτες, καθώς ο πατέρας δεν χρειάζεται να γράψει δεδομένα.
        close(from_server[i][0]);  // Κλείσιμο του pipe ανάγνωσης από τους πελάτες, καθώς ο πατέρας δεν χρειάζεται να διαβάσει από αυτούς.
    }

    char buffer[100];          // Δημιουργία buffer για την αποθήκευση μηνυμάτων προς τους πελάτες.
    for (int i = 0; i < 5 * 10; i++) {   // Επανάληψη για την επεξεργασία 50 αιτήσεων (5 πελάτες, 10 αιτήσεις ο καθένας).
        for (int j = 0; j < 5; j++) {   // Έλεγχος αιτήσεων για κάθε πελάτη.
            int product_id;
            if (read(to_server[j][0], &product_id, sizeof(product_id)) > 0) {  // Ανάγνωση του ID προϊόντος από το pipe που αντιστοιχεί σε έναν πελάτη
                total_orders++;  // Αυξάνουμε τον συνολικό αριθμό παραγγελιών
                if (catalog[product_id].item_count > 0) {   // Έλεγχος διαθεσιμότητας του προϊόντος στον κατάλογο.
                    catalog[product_id].item_count--;   // Μείωση του αποθέματος κατά 1.
                    snprintf(buffer, sizeof(buffer), "Order for Product %d (Price: %.2f) successful!", product_id, catalog[product_id].price); // Δημιουργία μηνύματος επιβεβαίωσης παραγγελίας με την τιμή
                    successful_orders++;  // Αυξάνουμε τον αριθμό των επιτυχημένων παραγγελιών
                    total_revenue += catalog[product_id].price;  // Προσθέτουμε την τιμή του προϊόντος στον συνολικό τζίρο
			sleep(1);
                } else {
                    snprintf(buffer, sizeof(buffer), "Product %d out of stock.", product_id);  // Δημιουργία μηνύματος αποτυχίας λόγω έλλειψης προϊόντος
                    failed_orders++;  // Αυξάνουμε τον αριθμό των αποτυχημένων παραγγελιών
			sleep(1);
                }
                write(from_server[j][1], buffer, sizeof(buffer));  // Αποστολή του μηνύματος προς τον πελάτη μέσω του αντίστοιχου pipe.
            }
			
        }
    }

    // Αναφορά
    printf("\nSummary Report:\n");       // Εκτύπωση συνολικής αναφοράς για τον κατάλογο.
    for (int i = 0; i < 20; i++) {       // Επανάληψη για όλα τα προϊόντα στον κατάλογο.
        printf("%s - Remaining: %d\n", catalog[i].description, catalog[i].item_count); // Εμφάνιση περιγραφής και υπολοίπου αποθέματος για κάθε προϊόν
    }

    // Συγκεντρωτική αναφορά
    printf("\nOverall Report:\n");
    printf("Total Orders: %d\n", total_orders);
    printf("Successful Orders: %d\n", successful_orders);
    printf("Failed Orders: %d\n", failed_orders);
    printf("Total Revenue: %.2f\n", total_revenue);

    return 0;
}
