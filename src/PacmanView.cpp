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

int main() {
    cout << instruction() << endl;
    return 0;
}