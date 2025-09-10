#include <iostream>

using namespace std;

string instruction() {
    return R"""(
    ------------------------------------ c ∩ ∩ ∩ ------------------------------------

    Bienvenido al juego de Pacman! Listo para el reto?
    Instrucciones:
    - (Jugador 1) Usa las flechas del teclado para mover a Pacman.
    - (Jugador 2) Usa las teclas W, A, S, D para mover a Pacman en la otra pantalla.
    - Recoge todos los puntos en el laberinto para ganar.
    - Evita a los fantasmas! Si te atrapan no hay vuelta atras, solo tienes 3 
      intentos.
    - En problemas? Toma una capsula de poder para destruir a los fantasmas por un 
      corto tiempo.
    - Come las frutas para ganar puntos extra!
    - Y lo mas importante, diviertete!

    ------------------------------------ c ∩ ∩ ∩ ------------------------------------
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

int main() {
    cout << instruction() << endl;
    return 0;
}