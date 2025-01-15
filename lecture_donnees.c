#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> // Pour gérer le temps

// Structures pour représenter les données
typedef struct {
    int n;       // Nombre d'objets
    int m;       // Nombre de contraintes
    int *profits; // Profits (c[j])
    int *capacities; // Capacités des contraintes (b[i])
    int **weights;   // Poids des objets pour chaque contrainte (a[i][j])
} ProblemData;

// Déclarations des fonctions
ProblemData* readData(const char* filename);
int isFeasible(ProblemData *data, int *solution);
void freeData(ProblemData *data);
void generateInitialSolutionGreedy(ProblemData *data, int *solution);
void printSolution(int *solution, int n);
int calculateProfit(ProblemData *data, int *solution);
void localSearch1Flip(ProblemData *data, int *solution, clock_t startTime, int maxTime);
void localSearchSwap(ProblemData *data, int *solution, clock_t startTime, int maxTime);
void vnd(ProblemData *data, int *solution, clock_t startTime, int maxTime);
void saveSolution(const char* filename, int *solution, int n, int profit);
void perturbSolution(ProblemData *data, int *solution, int k);
void vns(ProblemData *data, int *solution, clock_t startTime, int maxTime, int maxPerturbations);


// Fonction pour sauvegarder les résultats dans un fichier
void saveSolution(const char* filename, int *solution, int n, int profit) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Erreur lors de la création du fichier de sortie");
        exit(EXIT_FAILURE);
    }

    // Compter le nombre d'objets sélectionnés
    int selectedCount = 0;
    for (int i = 0; i < n; i++) {
        if (solution[i] == 1) {
            selectedCount++;
        }
    }

    // Écrire la valeur de l'objectif et le nombre d'objets
    fprintf(file, "%d %d\n", profit, selectedCount);

    // Écrire les indices des objets sélectionnés
    for (int i = 0; i < n; i++) {
        if (solution[i] == 1) {
            fprintf(file, "%d ", i + 1); // Les indices commencent à 1
        }
    }
    fprintf(file, "\n");

    fclose(file);
    printf("\nSolution sauvegardée dans '%s'\n", filename);
}

void perturbSolution(ProblemData *data, int *solution, int k) {
    // Perturber la solution en modifiant "k" objets aléatoires
    for (int i = 0; i < k; i++) {
        int index = rand() % data->n; // Choisir un objet aléatoire
        solution[index] = 1 - solution[index]; // Inverser l'état de l'objet
    }
}

void vns(ProblemData *data, int *solution, clock_t startTime, int maxTime, int maxPerturbations) {
    int *tempSolution = (int*)malloc(data->n * sizeof(int)); // Solution temporaire
    memcpy(tempSolution, solution, data->n * sizeof(int));

    int bestProfit = calculateProfit(data, solution);
    int currentProfit;

    for (int perturbation = 1; perturbation <= maxPerturbations; perturbation++) {
        if (((clock() - startTime) / CLOCKS_PER_SEC) >= maxTime) break;

        // Perturber la solution
        memcpy(tempSolution, solution, data->n * sizeof(int));
        perturbSolution(data, tempSolution, perturbation);

        // Optimiser la solution perturbée avec VND
        vnd(data, tempSolution, startTime, maxTime);

        // Calculer le profit de la solution perturbée
        currentProfit = calculateProfit(data, tempSolution);

        // Mise à jour si la solution perturbée est meilleure
        if (currentProfit > bestProfit) {
            memcpy(solution, tempSolution, data->n * sizeof(int));
            bestProfit = currentProfit;
            printf("\nNouvelle meilleure solution trouvée avec VNS : Profit = %d\n", bestProfit);
        }
    }

    free(tempSolution);
}


// Calculer le profit total d'une solution
int calculateProfit(ProblemData *data, int *solution) {
    int totalProfit = 0;
    for (int i = 0; i < data->n; i++) {
        if (solution[i] == 1) {
            totalProfit += data->profits[i];
        }
    }
    return totalProfit;
}

// Ajout des contraintes de temps dans les recherches locales et VND
void localSearch1Flip(ProblemData *data, int *solution, clock_t startTime, int maxTime) {
    int improved = 1;

    while (improved) {
        if (((clock() - startTime) / CLOCKS_PER_SEC) >= maxTime) break;
        improved = 0;
        int currentProfit = calculateProfit(data, solution);

        for (int i = 0; i < data->n; i++) {
            if (((clock() - startTime) / CLOCKS_PER_SEC) >= maxTime) break;
            solution[i] = 1 - solution[i]; // Flip (0 -> 1 ou 1 -> 0)
            if (isFeasible(data, solution)) {
                int newProfit = calculateProfit(data, solution);
                if (newProfit > currentProfit) {
                    currentProfit = newProfit;
                    improved = 1;
                } else {
                    solution[i] = 1 - solution[i]; // Annuler le flip
                }
            } else {
                solution[i] = 1 - solution[i]; // Annuler le flip
            }
        }
    }
}

void localSearchSwap(ProblemData *data, int *solution, clock_t startTime, int maxTime) {
    int improved = 1;

    while (improved) {
        if (((clock() - startTime) / CLOCKS_PER_SEC) >= maxTime) break;
        improved = 0;
        int currentProfit = calculateProfit(data, solution);

        for (int i = 0; i < data->n; i++) {
            if (solution[i] == 1) {
                for (int j = 0; j < data->n; j++) {
                    if (((clock() - startTime) / CLOCKS_PER_SEC) >= maxTime) break;
                    if (solution[j] == 0) {
                        solution[i] = 0;
                        solution[j] = 1;

                        if (isFeasible(data, solution)) {
                            int newProfit = calculateProfit(data, solution);
                            if (newProfit > currentProfit) {
                                currentProfit = newProfit;
                                improved = 1;
                                break;
                            } else {
                                solution[i] = 1;
                                solution[j] = 0;
                            }
                        } else {
                            solution[i] = 1;
                            solution[j] = 0;
                        }
                    }
                }
            }
            if (improved) break;
        }
    }
}

void vnd(ProblemData *data, int *solution, clock_t startTime, int maxTime) {
    int improved = 1;

    while (improved) {
        if (((clock() - startTime) / CLOCKS_PER_SEC) >= maxTime) break;
        improved = 0;

        int currentProfit = calculateProfit(data, solution);
        localSearch1Flip(data, solution, startTime, maxTime);
        int newProfit = calculateProfit(data, solution);

        if (newProfit > currentProfit) {
            improved = 1;
            continue;
        }

        currentProfit = newProfit;
        localSearchSwap(data, solution, startTime, maxTime);
        newProfit = calculateProfit(data, solution);

        if (newProfit > currentProfit) {
            improved = 1;
        }
    }
}

int main() {
    const char* filename = "100M5_1.txt"; // Nom du fichier d'entrée
    int maxTime = 20; // Limite de temps en secondes
    int maxPerturbations = 5; // Nombre maximum de perturbations
    clock_t startTime = clock(); // Démarrage du chronomètre

    // Lecture des données
    ProblemData *data = readData(filename);

    // Initialisation de la solution
    int *solution = (int*)calloc(data->n, sizeof(int));
    generateInitialSolutionGreedy(data, solution);

    printf("\nSolution initiale (gloutonne) :\n");
    printSolution(solution, data->n);
    printf("Profit initial : %d\n", calculateProfit(data, solution));

    // Recherche locale (1-flip)
    localSearch1Flip(data, solution, startTime, maxTime);
    printf("\nSolution après recherche locale (1-flip) :\n");
    printSolution(solution, data->n);
    printf("Profit après 1-flip : %d\n", calculateProfit(data, solution));

    // Recherche locale (échange)
    localSearchSwap(data, solution, startTime, maxTime);
    printf("\nSolution après recherche locale (échange) :\n");
    printSolution(solution, data->n);
    printf("Profit après échange : %d\n", calculateProfit(data, solution));

    // Descente en voisinage variable (VND)
    vnd(data, solution, startTime, maxTime);
    printf("\nSolution après descente en voisinage variable (VND) :\n");
    printSolution(solution, data->n);
    printf("Profit après VND : %d\n", calculateProfit(data, solution));

    // Recherche à voisinage variable (VNS)
    vns(data, solution, startTime, maxTime, maxPerturbations);
    printf("\nSolution finale après recherche à voisinage variable (VNS) :\n");
    printSolution(solution, data->n);
    printf("Profit final après VNS : %d\n", calculateProfit(data, solution));

    // Sauvegarde de la solution dans un fichier texte
    saveSolution("solution_vns.txt", solution, data->n, calculateProfit(data, solution));

    // Libération des ressources
    free(solution);
    freeData(data);

    return 0;
}

// Fonction pour lire les données depuis un fichier
ProblemData* readData(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier");
        exit(EXIT_FAILURE);
    }

    ProblemData *data = (ProblemData*)malloc(sizeof(ProblemData));
    if (!data) {
        perror("Erreur d'allocation mémoire");
        exit(EXIT_FAILURE);
    }

    fscanf(file, "%d %d", &data->n, &data->m);
    data->profits = (int*)malloc(data->n * sizeof(int));
    data->capacities = (int*)malloc(data->m * sizeof(int));
    data->weights = (int**)malloc(data->m * sizeof(int*));
    for (int i = 0; i < data->m; i++) {
        data->weights[i] = (int*)malloc(data->n * sizeof(int));
    }

    for (int i = 0; i < data->n; i++) fscanf(file, "%d", &data->profits[i]);
    for (int i = 0; i < data->m; i++) fscanf(file, "%d", &data->capacities[i]);
    for (int i = 0; i < data->m; i++) {
        for (int j = 0; j < data->n; j++) {
            fscanf(file, "%d", &data->weights[i][j]);
        }
    }

    fclose(file);
    return data;
}

// Fonction pour libérer la mémoire
void freeData(ProblemData *data) {
    free(data->profits);
    free(data->capacities);
    for (int i = 0; i < data->m; i++) {
        free(data->weights[i]);
    }
    free(data->weights);
    free(data);
}

// Fonction pour vérifier si une solution est faisable
int isFeasible(ProblemData *data, int *solution) {
    for (int i = 0; i < data->m; i++) {
        int weightSum = 0;
        for (int j = 0; j < data->n; j++) {
            if (solution[j] == 1) {
                weightSum += data->weights[i][j];
            }
        }
        if (weightSum > data->capacities[i]) {
            return 0; // Non faisable
        }
    }
    return 1; // Faisable
}

// Fonction pour générer une solution initiale gloutonne
void generateInitialSolutionGreedy(ProblemData *data, int *solution) {
    double *ratios = (double*)malloc(data->n * sizeof(double));
    for (int i = 0; i < data->n; i++) {
        ratios[i] = (double)data->profits[i] / data->weights[0][i]; // Ratio profit/poids
    }

    // Tri des objets par ratio décroissant
    for (int i = 0; i < data->n - 1; i++) {
        for (int j = i + 1; j < data->n; j++) {
            if (ratios[i] < ratios[j]) {
                double tempRatio = ratios[i];
                ratios[i] = ratios[j];
                ratios[j] = tempRatio;

                int tempProfit = data->profits[i];
                data->profits[i] = data->profits[j];
                data->profits[j] = tempProfit;

                for (int k = 0; k < data->m; k++) {
                    int tempWeight = data->weights[k][i];
                    data->weights[k][i] = data->weights[k][j];
                    data->weights[k][j] = tempWeight;
                }
            }
        }
    }

    // Sélection des objets dans l'ordre des ratios jusqu'à faisabilité
    for (int i = 0; i < data->n; i++) {
        solution[i] = 1;
        if (!isFeasible(data, solution)) {
            solution[i] = 0;
        }
    }

    free(ratios);
}

// Fonction pour afficher une solution
void printSolution(int *solution, int n) {
    printf("Solution : ");
    for (int i = 0; i < n; i++) {
        printf("%d ", solution[i]);
    }
    printf("\n");
}

