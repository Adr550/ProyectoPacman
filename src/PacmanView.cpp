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
                bool power_up_activo = false;
                for (const auto& power_up : power_ups) {
                    if (power_up.x == j && power_up.y == i) {
                        power_up_activo = true;
                        break;
                    }
                }
                if (power_up_activo) {
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

void procesar_colision(Posicion& jugador, int& vidas_jugador, int& puntos_jugador) {
    for (int i = 0; i < fantasmas.size(); i++) {
        if (fantasmas[i] == jugador) {
            if (power_up_activo) {
                puntos_jugador += 200;
                fantasmas[i] = Posicion(6 + rand() % 2, 3 + rand() % 2);
            } else {
                vidas_jugador--;
                if (vidas_jugador <= 0) {
                    game_over = true;
                } else {
                    jugador = (modo_2jugadores && &jugador == &pacman2) ? 
                             Posicion(11, 5) : Posicion(7, 5);
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
        
        // Procesar colisión
        procesar_colision(jugador, (&jugador == &pacman) ? vidas : vidas2, puntos_jugador);
    }
    
    pthread_mutex_unlock(&mutex_juego);
}

void mover_fantasma(int indice) {
    pthread_mutex_lock(&mutex_juego);
    
    int direcciones[4][2] = {{0,1}, {1,0}, {0,-1}, {-1,0}};
    vector<int> direcciones_validas;
    
    for (int i = 0; i < 4; i++) {
        int nuevo_x = fantasmas[indice].x + direcciones[i][0];
        int nuevo_y = fantasmas[indice].y + direcciones[i][1];
        
        if (es_movimiento_valido(nuevo_x, nuevo_y)) {
            direcciones_validas.push_back(i);
        }
    }
    
    if (!direcciones_validas.empty()) {
        int dir_aleatoria = direcciones_validas[rand() % direcciones_validas.size()];
        fantasmas[indice].x += direcciones[dir_aleatoria][0];
        fantasmas[indice].y += direcciones[dir_aleatoria][1];
        aplicar_teletransporte(fantasmas[indice]);
    }
    
    pthread_mutex_unlock(&mutex_juego);
}

void* hilo_fantasma(void* arg) {
    int indice = *(int*)arg;
    
    while (!game_over) {
        mover_fantasma(indice);
        usleep(400000 + (rand() % 200000));
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
    
    // Pantalla final ultra compacta
    clear();
    if (puntos.size() == 0 && power_ups.size() == 0) {
        mvprintw(3, 5, "GANASTE!");
    } else {
        mvprintw(3, 5, "GAME OVER");
    }
    
    if (modo_2jugadores) {
        mvprintw(4, 3, "J1:%d J2:%d", puntuacion, puntuacion2);
    } else {
        mvprintw(4, 5, "P:%d", puntuacion);
    }
    
    mvprintw(6, 3, "ENTER continuar");
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