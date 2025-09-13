#include <iostream>
#include <ncurses.h>  // getch()
// #include <conio.h> Para kbhit() en Windows.
// Para que lo detecte el sistema:
// sudo apt-get install libncurses5-dev libncursesw5-dev

using namespace std;

string instruction() {
    return R"""(
-------------------------------------- c ∩ ∩ ∩ --------------------------------------

Bienvenido al juego de Pacman! Listo para el reto?
Instrucciones:
- (Jugador 1) Usa las flechas del teclado para mover a Pacman.
- (Jugador 2) Usa las teclas W, A, S, D para mover a Pacman en la otra pantalla.
- Recoge todos los puntos en el laberinto para ganar.
- Evita a los fantasmas! Si te atrapan no hay vuelta atras, solo tienes 3 intentos.
- En problemas? Toma una capsula de poder para destruir a los fantasmas por un corto 
  tiempo.
- Come las frutas para ganar puntos extra!
- Y lo mas importante, diviertete!

Presiona ENTER para volver al menu principal.

-------------------------------------- c ∩ ∩ ∩ --------------------------------------
    )""";
}

string Puntajes (string nombre1, string nombre2, int puntaje1, int puntaje2, string nombre3, int puntaje3,
    string nombre4, int puntaje4, string nombre5, int puntaje5) {
    return R"""(
    ------------------------------------ c ∩ ∩ ∩ ------------------------------------

                                      LeaderBoards
    1. )""" + nombre1 + " " + to_string(puntaje1) + R"""(

    2. )""" + nombre2 + " " + to_string(puntaje2) + R"""(

    3. )""" + nombre3 + " " + to_string(puntaje3) + R"""(

    4. )""" + nombre4 + " " + to_string(puntaje4) + R"""(
    
    5. )""" + nombre5 + " " + to_string(puntaje5) + R"""(

    ------------------------------------ c ∩ ∩ ∩ ------------------------------------
    )""";
}

// Función de logo
void Logo() {
    printw("    ██████╗  █████╗  ██████╗███╗   ███╗ █████╗ ███╗   ██╗\n");
    printw("    ██╔══██╗██╔══██╗██╔════╝████╗ ████║██╔══██╗████╗  ██║\n");
    printw("    ██████╔╝███████║██║     ██╔████╔██║███████║██╔██╗ ██║\n");
    printw("    ██╔═══╝ ██╔══██║██║     ██║╚██╔╝██║██╔══██║██║╚██╗██║\n");
    printw("    ██║     ██║  ██║╚██████╗██║ ╚═╝ ██║██║  ██║██║ ╚████║\n");
    printw("    ╚═╝     ╚═╝  ╚═╝ ╚═════╝╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝\n");
    printw("\n");
    printw("                    ● ● ● c ∩ ∩ ∩ ● ● ●\n\n");
}

// Función opciones del menu
void Opciones(string opciones[], int tamano, int seleccionado) {
    for (int i = 0; i < tamano; i++) {
        if (i == seleccionado) {
            attron(A_REVERSE); // Resaltar opción seleccionada
            printw("    ► %s ◄\n", opciones[i].c_str());
            attroff(A_REVERSE);
        } else {
            printw("      %s\n", opciones[i].c_str());
        }
    }
}

int main() {
    // Inicializar ncurses
    initscr();
    noecho();
    keypad(stdscr, TRUE);
    
    string opciones[4] = {"Jugar", "Instrucciones", "Puntajes", "Salir"};
    int seleccionado = 0;
    int tecla;
    bool salir = false;
    
    while (!salir) {
        // Limpiar pantalla y mostrar menú
        clear();
        
        Logo();
        Opciones(opciones, 4, seleccionado);
        
        printw("\n\n    Usa W/S para navegar y ENTER para seleccionar\n");
        printw("                    ESC para salir\n");
        
        refresh();
        
        // Leer tecla
        tecla = getch();
        
        switch (tecla) {
            case 'w':
            case 'W':
                if (seleccionado > 0) {
                    seleccionado--;
                }
                break;
                
            case 's':
            case 'S':
                if (seleccionado < 3) {  // 4 opciones = indices 0-3
                    seleccionado++;
                }
                break;
                
            case 10:  
                clear();
                switch (seleccionado) {
                    case 0: // Jugar
                        printw("Juego no se ha hecho aún :( \nPresiona ENTER para volver.\n");
                        refresh();
                        while (getch() != 10) { /* espera el enter */ }
                        break;
                    case 1: // Instrucciones 
                        printw("%s", instruction().c_str());
                        refresh();
                        while (getch() != 10) { /* espera el enter */ }
                        break;
                    case 2: // Puntajes
                        printw("Puntajes por hacer \nPresiona ENTER para volver.\n");
                        refresh();
                        while (getch() != 10) { /* espera el enter */ }
                        break;
                    case 3: // Salir
                        salir = true;
                        break;
                }
                break;
                
            case 27: // esc
                salir = true;
                break;
        }
    }
    
    // Finaliza
    endwin();
    cout << "Gracias por jugar Pacman" << endl;
    return 0;
}