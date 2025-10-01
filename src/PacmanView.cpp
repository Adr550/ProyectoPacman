#include <iostream>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <algorithm>

using namespace std;

// Constantes del juego - más compactas
const int FILAS = 11;
const int COLUMNAS = 15;
const int PUNTOS_POWER_UP = 50;
const int TIEMPO_POWER_UP = 10;

// Estructura para posición
struct Posicion {
    int x, y;
    Posicion(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Posicion& other) const {
        return x == other.x && y == other.y;
    }
};

// Estructura para puntuaciones
struct Puntuacion {
    string nombre;
    int puntos;
    Puntuacion(string n = "Jugador", int p = 0) : nombre(n), puntos(p) {}
};

// Variables globales del juego
char mapa[FILAS][COLUMNAS];
int puntuacion = 0;
int vidas = 3;
bool game_over = false;
bool power_up_activo = false;
int tiempo_power_up = 0;
Posicion pacman;
vector<Posicion> fantasmas;
vector<Posicion> puntos;
vector<Posicion> power_ups;
vector<pthread_t> hilos;
vector<Puntuacion> leaderboard;

// Variables para modo 2 jugadores
bool modo_2jugadores = false;
Posicion pacman2;
int puntuacion2 = 0;
int vidas2 = 3;

// NUEVAS VARIABLES para sistema de dificultad
int tiempo_juego = 0;
bool fantasmas_activos[4] = {true, false, false, false};
int velocidad_fantasmas[4] = {500000, 500000, 500000, 300000};

// Mutex para sincronización
pthread_mutex_t mutex_juego = PTHREAD_MUTEX_INITIALIZER;

// Mapa ultra compacto de Pacman (11x15)
const char mapa_original[FILAS][COLUMNAS + 1] = {
    "###############",
    "#.....#.....#.#",
    "#.#.#...#.#...#",
    "#*...........*#",
    "#.#.#.#.#.#.#.#",
    "#.............#",
    "###.#.....#####",
    "   #.....#     ",
    "#####.#.##### #",
    "#.............#",
    "###############"
};

// Posiciones de los power-ups
const Posicion posiciones_power_up[] = {
    Posicion(1, 3),
    Posicion(13, 3)
};

// Funciones compactas del menú
void mostrarInstrucciones() {
    clear();
    printw("INSTRUCCIONES:\n\n");
    printw("J1: Flechas mover\n");
    printw("J2: WASD mover\n\n");
    printw("Objetivo:\n");
    printw("- Come puntos (.)\n");
    printw("- Power-ups (*)\n");
    printw("- Evita fantasmas\n\n");
    printw("R:Reiniciar Q:Menu\n");
    printw("\nENTER volver");
    refresh();
    while (getch() != 10);
}

void cargarPuntajes() {
    leaderboard.clear();
    leaderboard.push_back(Puntuacion("PAC-MASTER", 5000));
    leaderboard.push_back(Puntuacion("GHOST-HUNTER", 3500));
    leaderboard.push_back(Puntuacion("NEWBIE", 2000));
    leaderboard.push_back(Puntuacion("PLAYER", 1000));
    leaderboard.push_back(Puntuacion("BEGINNER", 500));
}

void guardarPuntaje(string nombre, int puntos) {
    leaderboard.push_back(Puntuacion(nombre, puntos));
    sort(leaderboard.begin(), leaderboard.end(), 
         [](const Puntuacion& a, const Puntuacion& b) { 
             return a.puntos > b.puntos; 
         });
    if (leaderboard.size() > 5) {
        leaderboard.pop_back();
    }
}

void mostrarPuntajes() {
    clear();
    printw("PUNTAJES:\n\n");
    
    for (int i = 0; i < leaderboard.size(); i++) {
        printw("%d.%-8s%d\n", 
               i + 1, 
               leaderboard[i].nombre.c_str(), 
               leaderboard[i].puntos);
    }
    
    printw("\nENTER volver");
    refresh();
    while (getch() != 10);
}

void Logo() {
    printw(" PACMAN\n");
    printw(" c ∩ ∩ ∩\n\n");
}

void Opciones(string opciones[], int tamano, int seleccionado) {
    for (int i = 0; i < tamano; i++) {
        if (i == seleccionado) {
            attron(A_REVERSE);
            printw(">%s<\n", opciones[i].c_str());
            attroff(A_REVERSE);
        } else {
            printw(" %s\n", opciones[i].c_str());
        }
    }
}

// Funciones del juego
void inicializar_mapa() {
    puntos.clear();
    power_ups.clear();
    
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLUMNAS; j++) {
            mapa[i][j] = mapa_original[i][j];
            if (mapa[i][j] == '.') {
                puntos.push_back(Posicion(j, i));
            } else if (mapa[i][j] == '*') {
                power_ups.push_back(Posicion(j, i));
            }
        }
    }
}

void inicializar_personajes() {
    if (modo_2jugadores) {
        pacman = Posicion(3, 5);
        pacman2 = Posicion(11, 5);
    } else {
        pacman = Posicion(7, 5);
    }
    
    fantasmas.clear();
    fantasmas.push_back(Posicion(6, 3));
    fantasmas.push_back(Posicion(7, 3));
    fantasmas.push_back(Posicion(6, 4));
    fantasmas.push_back(Posicion(7, 4));
    
    // Resetear sistema de dificultad
    tiempo_juego = 0;
    fantasmas_activos[0] = true;
    fantasmas_activos[1] = false;
    fantasmas_activos[2] = false;
    fantasmas_activos[3] = false;
    velocidad_fantasmas[0] = 500000;
    velocidad_fantasmas[1] = 500000;
    velocidad_fantasmas[2] = 500000;
    velocidad_fantasmas[3] = 300000;
}

void dibujar_mapa() {
    clear();
    
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLUMNAS; j++) {
            char caracter = ' ';
            bool dibujar = true;
            
            if (mapa[i][j] == '#') {
                attron(COLOR_PAIR(1));
                caracter = '#';
            } else if (mapa[i][j] == '.') {
                bool punto_activo = false;
                for (const auto& punto : puntos) {
                    if (punto.x == j && punto.y == i) {
                        punto_activo = true;
                        break;
                    }
                }
                if (punto_activo) {
                    attron(COLOR_PAIR(3));
                    caracter = '.';
                } else {
                    dibujar = false;
                }
            } else if (mapa[i][j] == '*') {
                bool power_up_activo_mapa = false;
                for (const auto& power_up : power_ups) {
                    if (power_up.x == j && power_up.y == i) {
                        power_up_activo_mapa = true;
                        break;
                    }
                }
                if (power_up_activo_mapa) {
                    attron(COLOR_PAIR(9));
                    caracter = '*';
                } else {
                    dibujar = false;
                }
            } else {
                dibujar = false;
            }
            
            if (dibujar) {
                mvaddch(i, j, caracter);
                attroff(COLOR_PAIR(1) | COLOR_PAIR(3) | COLOR_PAIR(9));
            } else {
                mvaddch(i, j, ' ');
            }
        }
    }
    
    // Dibujar Pacman(s)
    attron(COLOR_PAIR(2));
    mvaddch(pacman.y, pacman.x, 'C');
    attroff(COLOR_PAIR(2));
    
    if (modo_2jugadores) {
        attron(COLOR_PAIR(10));
        mvaddch(pacman2.y, pacman2.x, 'P');
        attroff(COLOR_PAIR(10));
    }
    
    // Dibujar fantasmas
    for (int i = 0; i < fantasmas.size(); i++) {
        if (power_up_activo) {
            attron(COLOR_PAIR(4));
        } else {
            attron(COLOR_PAIR(i + 5));
        }
        mvaddch(fantasmas[i].y, fantasmas[i].x, 'G');
        attroff(COLOR_PAIR(4) | COLOR_PAIR(5) | COLOR_PAIR(6) | COLOR_PAIR(7) | COLOR_PAIR(8));
    }
    
    // Información ultra compacta
    if (modo_2jugadores) {
        mvprintw(FILAS, 0, "J1:%dL%d", puntuacion, vidas);
        mvprintw(FILAS, 8, "J2:%dL%d", puntuacion2, vidas2);
    } else {
        mvprintw(FILAS, 0, "P:%d L:%d", puntuacion, vidas);
    }
    
    if (power_up_activo) {
        mvprintw(FILAS, COLUMNAS-6, "PWR:%d", tiempo_power_up);
    }
    
    refresh();
}

bool es_movimiento_valido(int x, int y) {
    if (x < 0 || x >= COLUMNAS || y < 0 || y >= FILAS) {
        return false;
    }
    return mapa[y][x] != '#';
}

void aplicar_teletransporte(Posicion& pos) {
    if (pos.y == 7) {
        if (pos.x <= 0) {
            pos.x = COLUMNAS - 2;
        } else if (pos.x >= COLUMNAS - 1) {
            pos.x = 1;
        }
    }
}

void colision(Posicion& jugador, int& vidas_jugador, int& puntos_jugador, bool es_jugador2 = false) {
    for (int i = 0; i < fantasmas.size(); i++) {
        if (fantasmas[i] == jugador) {
            if (power_up_activo) {
                puntos_jugador += 200;
                fantasmas[i] = Posicion(6 + rand() % 2, 3 + rand() % 2);
            } else {
                vidas_jugador--;
                if (vidas_jugador <= 0) {
                    // En modo 2 jugadores verificar si el otro jugador sigue vivo
                    if (modo_2jugadores) {
                        int& vidas_otro = es_jugador2 ? vidas : vidas2;
                        if (vidas_otro <= 0) {
                            game_over = true; // ambos pierden 
                        }
                    } else {
                        game_over = true;
                    }
                } else {
                    // reposicionar al jugador en su punto de inicio
                    if (es_jugador2) {
                        jugador = Posicion(11, 5);
                    } else {
                        jugador = modo_2jugadores ? Posicion(3, 5) : Posicion(7, 5);
                    }
                    pthread_mutex_unlock(&mutex_juego);
                    usleep(800000);
                    pthread_mutex_lock(&mutex_juego);
                }
            }
            break;
        }
    }
}

void mover_pacman(int direccion, Posicion& jugador, int& puntos_jugador) {
    pthread_mutex_lock(&mutex_juego);
    
    int nuevo_x = jugador.x;
    int nuevo_y = jugador.y;
    
    switch(direccion) {
        case KEY_UP: nuevo_y--; break;
        case KEY_DOWN: nuevo_y++; break;
        case KEY_LEFT: nuevo_x--; break;
        case KEY_RIGHT: nuevo_x++; break;
        case 'w': case 'W': if (modo_2jugadores) nuevo_y--; break;
        case 's': case 'S': if (modo_2jugadores) nuevo_y++; break;
        case 'a': case 'A': if (modo_2jugadores) nuevo_x--; break;
        case 'd': case 'D': if (modo_2jugadores) nuevo_x++; break;
    }
    
    if (es_movimiento_valido(nuevo_x, nuevo_y)) {
        jugador.x = nuevo_x;
        jugador.y = nuevo_y;
        
        aplicar_teletransporte(jugador);
        
        // Comer puntos
        for (auto it = puntos.begin(); it != puntos.end(); ) {
            if (*it == jugador) {
                puntos_jugador += 10;
                it = puntos.erase(it);
            } else {
                ++it;
            }
        }
        
        // Comer power-ups
        for (auto it = power_ups.begin(); it != power_ups.end(); ) {
            if (*it == jugador) {
                power_up_activo = true;
                tiempo_power_up = TIEMPO_POWER_UP;
                puntos_jugador += PUNTOS_POWER_UP;
                it = power_ups.erase(it);
            } else {
                ++it;
            }
        }
        
        // procesar colisión cambios
        bool es_jugador2 = (&jugador == &pacman2);
        int& vidas_actual = es_jugador2 ? vidas2 : vidas;
        colision(jugador, vidas_actual, puntos_jugador, es_jugador2);
    }
    
    pthread_mutex_unlock(&mutex_juego);
}

//Calcular distancia Manhattan
int distancia_manhattan(Posicion a, Posicion b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

//Fantasmas persiguen a pacman
void moverfantasma(int indice) {
    pthread_mutex_lock(&mutex_juego);
    
    // Determinar objetivo, el pacman mas cercano
    Posicion objetivo = pacman;
    if (modo_2jugadores) {
        int dist1 = distancia_manhattan(fantasmas[indice], pacman);
        int dist2 = distancia_manhattan(fantasmas[indice], pacman2);
        if (dist2 < dist1) {
            objetivo = pacman2;
        }
    }
    
    int direcciones[4][2] = {{0,1}, {1,0}, {0,-1}, {-1,0}};
    int mejor_direccion = -1;
    int menor_distancia = 9999;
    
    // Encuentra la dirección que acerca más al objetivo
    for (int i = 0; i < 4; i++) {
        int nuevo_x = fantasmas[indice].x + direcciones[i][0];
        int nuevo_y = fantasmas[indice].y + direcciones[i][1];
        
        if (es_movimiento_valido(nuevo_x, nuevo_y)) {
            Posicion nueva_pos(nuevo_x, nuevo_y);
            int distancia = distancia_manhattan(nueva_pos, objetivo);
            
            if (distancia < menor_distancia) {
                menor_distancia = distancia;
                mejor_direccion = i;
            }
        }
    }
    
    // Si hay power-up activo huye en lugar de perseguir
    if (power_up_activo && mejor_direccion != -1) {
        vector<int> direcciones_escape;
        for (int i = 0; i < 4; i++) {
            int nuevo_x = fantasmas[indice].x + direcciones[i][0];
            int nuevo_y = fantasmas[indice].y + direcciones[i][1];
            
            if (es_movimiento_valido(nuevo_x, nuevo_y)) {
                Posicion nueva_pos(nuevo_x, nuevo_y);
                int distancia = distancia_manhattan(nueva_pos, objetivo);
                
                if (distancia > distancia_manhattan(fantasmas[indice], objetivo)) {
                    direcciones_escape.push_back(i);
                }
            }
        }
        
        if (!direcciones_escape.empty()) {
            mejor_direccion = direcciones_escape[rand() % direcciones_escape.size()];
        }
    }
    
    // Mover al fantasma
    if (mejor_direccion != -1) {
        fantasmas[indice].x += direcciones[mejor_direccion][0];
        fantasmas[indice].y += direcciones[mejor_direccion][1];
        aplicar_teletransporte(fantasmas[indice]);
    }
    
    pthread_mutex_unlock(&mutex_juego);
}

// Hilo del fantasma con velocidad variable
void* hilo_fantasma(void* arg) {
    int indice = *(int*)arg;
    
    while (!game_over) {
        if (fantasmas_activos[indice]) {
            moverfantasma(indice);
        }
        
        // Si hay power-up, los fantasmas se vuelven SUPER lentos
        if (power_up_activo) {
            usleep(1000000); // 1 segundo (muy lento)
        } else {
            usleep(velocidad_fantasmas[indice]);
        }
    }
    
    return NULL;
}

void* hilo_power_up(void* arg) {
    while (!game_over) {
        sleep(1);
        pthread_mutex_lock(&mutex_juego);
        if (power_up_activo) {
            tiempo_power_up--;
            if (tiempo_power_up <= 0) {
                power_up_activo = false;
            }
        }
        pthread_mutex_unlock(&mutex_juego);
    }
    return NULL;
}

// Control de dificultad progresiva
void* hilo_dificultad(void* arg) {
    while (!game_over) {
        sleep(1);
        pthread_mutex_lock(&mutex_juego);
        
        tiempo_juego++;
        
        // A los 10 segundos activar fantasma 2
        if (tiempo_juego == 10 && !fantasmas_activos[1]) {
            fantasmas_activos[1] = true;
        }
        
        // A los 20 segundos activar fantasma 3
        if (tiempo_juego == 20 && !fantasmas_activos[2]) {
            fantasmas_activos[2] = true;
        }
        
        // A los 30 segundos activar fantasma 4 (el rápido)
        if (tiempo_juego == 30 && !fantasmas_activos[3]) {
            fantasmas_activos[3] = true;
        }
        
        // A los 40 segundos aumentar velocidad de todos
        if (tiempo_juego == 40) {
            for (int i = 0; i < 3; i++) {
                velocidad_fantasmas[i] = 350000;
            }
            velocidad_fantasmas[3] = 250000;
        }
        
        // A los 60 segundos modo difícil extremo
        if (tiempo_juego == 60) {
            for (int i = 0; i < 3; i++) {
                velocidad_fantasmas[i] = 300000;
            }
            velocidad_fantasmas[3] = 200000;
        }
        
        pthread_mutex_unlock(&mutex_juego);
    }
    return NULL;
}

void* hilo_entrada_usuario(void* arg) {
    int ch;
    while (!game_over) {
        ch = getch();
        if (ch != ERR) {
            if (ch == 'r' || ch == 'R') {
                pthread_mutex_lock(&mutex_juego);
                inicializar_mapa();
                inicializar_personajes();
                puntuacion = 0;
                puntuacion2 = 0;
                vidas = 3;
                vidas2 = 3;
                game_over = false;
                power_up_activo = false;
                pthread_mutex_unlock(&mutex_juego);
            } else if (ch == 'q' || ch == 'Q') {
                game_over = true;
            } else {
                if (modo_2jugadores) {
                    if (ch == KEY_UP || ch == KEY_DOWN || ch == KEY_LEFT || ch == KEY_RIGHT) {
                        mover_pacman(ch, pacman, puntuacion);
                    } else if (ch == 'w' || ch == 'W' || ch == 's' || ch == 'S' || 
                               ch == 'a' || ch == 'A' || ch == 'd' || ch == 'D') {
                        mover_pacman(ch, pacman2, puntuacion2);
                    }
                } else {
                    mover_pacman(ch, pacman, puntuacion);
                }
            }
        }
        usleep(80000);
    }
    return NULL;
}

void inicializar_colores() {
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_RED, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_CYAN, COLOR_BLACK);
    init_pair(8, COLOR_GREEN, COLOR_BLACK);
    init_pair(9, COLOR_YELLOW, COLOR_BLACK);
    init_pair(10, COLOR_GREEN, COLOR_BLACK);
}

void jugar() {
    puntuacion = 0;
    puntuacion2 = 0;
    vidas = 3;
    vidas2 = 3;
    game_over = false;
    power_up_activo = false;
    
    inicializar_mapa();
    inicializar_personajes();
    
    hilos.clear();
    
    // Crear hilos
    int indices_fantasmas[4] = {0, 1, 2, 3};
    for (int i = 0; i < 4; i++) {
        pthread_t hilo;
        pthread_create(&hilo, NULL, hilo_fantasma, &indices_fantasmas[i]);
        hilos.push_back(hilo);
    }
    
    pthread_t hilo_power;
    pthread_create(&hilo_power, NULL, hilo_power_up, NULL);
    hilos.push_back(hilo_power);
    
    // Hilo de dificultad progresiva
    pthread_t hilo_dif;
    pthread_create(&hilo_dif, NULL, hilo_dificultad, NULL);
    hilos.push_back(hilo_dif);
    
    pthread_t hilo_entrada;
    pthread_create(&hilo_entrada, NULL, hilo_entrada_usuario, NULL);
    hilos.push_back(hilo_entrada);
    
    // Bucle principal del juego
    while (!game_over && (puntos.size() > 0 || power_ups.size() > 0)) {
        dibujar_mapa();
        usleep(120000);
    }
    
    game_over = true;
    
    for (auto& hilo : hilos) {
        pthread_join(hilo, NULL);
    }
    
    // Pantalla final mejorada - MEJORA APLICADA
    clear();
    
    if (puntos.size() == 0 && power_ups.size() == 0) {
        attron(COLOR_PAIR(2));
        mvprintw(2, 4, "VICTORIA!");
        attroff(COLOR_PAIR(2));
    } else {
        attron(COLOR_PAIR(5));
        mvprintw(2, 3, "GAME OVER");
        attroff(COLOR_PAIR(5));
    }
    
    if (modo_2jugadores) {
        mvprintw(4, 1, "Jugador 1: %d pts", puntuacion);
        mvprintw(5, 1, "Jugador 2: %d pts", puntuacion2);
        
        // Determinar ganador
        if (puntuacion > puntuacion2) {
            attron(COLOR_PAIR(2));
            mvprintw(7, 2, "Gana J1!");
            attroff(COLOR_PAIR(2));
        } else if (puntuacion2 > puntuacion) {
            attron(COLOR_PAIR(10));
            mvprintw(7, 2, "Gana J2!");
            attroff(COLOR_PAIR(10));
        } else {
            mvprintw(7, 3, "EMPATE!");
        }
    } else {
        mvprintw(4, 3, "Puntos: %d", puntuacion);
    }
    
    mvprintw(9, 2, "ENTER continuar");
    refresh();
    
    nodelay(stdscr, FALSE);
    while (getch() != 10);
    
    // Guardar puntuación si es modo un jugador
    if (!modo_2jugadores && puntuacion > 0) {
        clear();
        echo();
        char nombre[15];
        mvprintw(3, 3, "Nombre:");
        refresh();
        getnstr(nombre, 14);
        noecho();
        guardarPuntaje(nombre, puntuacion);
    }
}

int main() {
    srand(time(NULL));
    cargarPuntajes();
    
    // Inicializar ncurses
    initscr();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    // Verificar tamaño del terminal - ahora más permisivo
    int filas, columnas;
    getmaxyx(stdscr, filas, columnas);
    if (filas < 13 || columnas < 17) {
        endwin();
        cout << "Terminal muy pequeño." << endl;
        cout << "Necesitas al menos 13x17" << endl;
        cout << "Tienes: " << filas << "x" << columnas << endl;
        return 1;
    }
    
    inicializar_colores();
    
    // Menú principal ultra compacto
    string opciones[5] = {"1 Jugador", "2 Jugadores", "Instrucciones", "Puntajes", "Salir"};
    int seleccionado = 0;
    int tecla;
    bool salir = false;
    
    while (!salir) {
        clear();
        
        Logo();
        printw("\n");
        Opciones(opciones, 5, seleccionado);
        
        printw("\nW/S + ENTER");
        printw("\nESC: Salir");
        
        refresh();
        
        tecla = getch();
        
        switch (tecla) {
            case 'w':
            case 'W':
                if (seleccionado > 0) seleccionado--;
                break;
                
            case 's':
            case 'S':
                if (seleccionado < 4) seleccionado++;
                break;
                
            case 10:
                clear();
                switch (seleccionado) {
                    case 0:
                        modo_2jugadores = false;
                        nodelay(stdscr, TRUE);
                        jugar();
                        nodelay(stdscr, FALSE);
                        break;
                    case 1:
                        modo_2jugadores = true;
                        nodelay(stdscr, TRUE);
                        jugar();
                        nodelay(stdscr, FALSE);
                        break;
                    case 2:
                        mostrarInstrucciones();
                        break;
                    case 3:
                        mostrarPuntajes();
                        break;
                    case 4:
                        salir = true;
                        break;
                }
                break;
                
            case 27:
                salir = true;
                break;
        }
    }
    
    endwin();
    return 0;
}