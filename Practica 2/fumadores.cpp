/*
* Práctica 2 - Sistemas Concurrentes y Distribuidos
* Problema de los Fumadores implementado con Monitores de Hoare
* y señales SU (Señalar y Espera Urgente)
*
* autor: Francisco Domínguez Lorente
*/

#include <iostream>
#include <cassert>
#include <iomanip>
#include <thread>
#include <random>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

/* Variables globales */
const int num_fumadores = 3;
mutex mtx;

/* Plantilla para generar un entero aleatorio entre dos números, ambos incluidos */
template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max );
  return distribucion_uniforme( generador );
}

/* Función que simula la acción de fumar con un retardo aleatorio */
void fumar(int num_fumador)
{
  chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

  mtx.lock();
  cout << "\t\tFumador " << num_fumador << ": empieza a fumar ("
       << duracion_fumar.count() << " milisegundos)..." << endl;
  mtx.unlock();

  this_thread::sleep_for(duracion_fumar);

  mtx.lock();
  cout << "\t\tFumador " << num_fumador << ": termina de fumar, comienza espera de ingrediente "
       << num_fumador << "." << endl;
  mtx.unlock();
}

/* Función que simula la acción de poner un ingrediente en el mostrador
Devuelve un entero entre 0 y 2 */
int producirIngrediente()
{
  this_thread::sleep_for(chrono::milliseconds( aleatorio<20,200>() ));
  return aleatorio<0,num_fumadores-1>();
}

/* Clase del Monitor para controlar el estanco */
class Estanco : public HoareMonitor {
  private:
    int ingrediente;
    CondVar mostrador_vacio;
    CondVar ingrediente_disponible[num_fumadores];

  public:
    Estanco();
    void obtenerIngrediente(int i);
    void ponerIngrediente(int i);
    void esperarRecogidaIngrediente();
};

/* Constructor por defecto */
Estanco::Estanco()
{
  /* Como la función producirIngrediente() devuelve un entero entre 0 y 2
  tenemos que inicializar la variable a otro valor distinto para saber
  si tenemos o no ingrediente */
  ingrediente = -1;
  mostrador_vacio = newCondVar();

  for (int i=0; i<num_fumadores; i++){
    ingrediente_disponible[i] = newCondVar();
  }
}

/* Función obtenerIngrediente(). El fumador espera bloqueado a que
su ingrediente esté disponible y luego lo retira del mostrador*/
void Estanco::obtenerIngrediente(int i)
{
  /* Espera a que el ingrediente esté disponible */
  if (ingrediente != i){
    ingrediente_disponible[i].wait();
  }

  /* Vuelve a ponerlo a -1 para indicar que ya no hay ingrediente */
  ingrediente = -1;

  mtx.lock();
  cout << "\t\tRetirado ingrediente: " << i << endl;
  mtx.unlock();

  /* Se ha retirado el ingrediente, ahora el mostrador está vacío */
  mostrador_vacio.signal();
}

/* Función ponerIngrediente. Pone el ingrediente i en el mostrador */
void Estanco::ponerIngrediente(int i)
{
  ingrediente = i;

  mtx.lock();
  cout << "Puesto el ingrediente: " << ingrediente << endl;
  mtx.unlock();

  /* Envía una señal de que el ingrediente i está disponible */
  ingrediente_disponible[i].signal();
}

/* Función llamada por el estanquero en cada iteración de su bucle infinito.
Espera bloqueado hasta que el mostrador esté libre */
void Estanco::esperarRecogidaIngrediente()
{
  /* Bloqueado hasta que ingrediente sea -1, es decir, que no haya ingrediente */
  if (ingrediente != -1){
    mostrador_vacio.wait();
  }
}

/* Función hebra del fumador. En cada iteración del bucle infinito:
    - Llama al procedimiento del monitor obtenerIngrediente(i), donde i es el número
    de fumador o número de ingrediente que esperan. Espera bloqueado hasta que el
    ingrediente esté disponible y lo retira
    - Llama a la función fumar, que es una espera aleatoria */
void funcion_hebra_fumador(MRef<Estanco> monitor, int i)
{
  while(true)
  {
    monitor->obtenerIngrediente(i);
    fumar(i);
  }
}

/* Función hebra del estanquero. En cada iteración del bucle infinito:
    - Produce un ingrediente aleatorio llamando a la función producirIngrediente()
    - Llama al procedimiento del monitor ponerIngrediente(), que pone el ingrediente en el mostrador
    - Llama al procedimiento del monitor esperarRecogidaIngrediente(), que espera
    bloqueado hasta que el mostrador esté libre */
void funcion_hebra_estanquero(MRef<Estanco> monitor)
{
   while(true)
   {
     int ingred = producirIngrediente();
     monitor->ponerIngrediente(ingred);
     monitor->esperarRecogidaIngrediente();
   }
}

int main()
{
  cout << endl << "--------------------------------------------------------" << endl
               << "Problema de los fumadores" << endl
               << "--------------------------------------------------------" << endl
               << flush ;

  MRef<Estanco> monitor = Create<Estanco>();

  thread hebra_estanquero(funcion_hebra_estanquero);
  thread hebra_fumador[num_fumadores];

  for(int i=0; i<num_fumadores; i++){
    hebra_fumador[i] = thread(funcion_hebra_fumador, monitor, i);
  }

  for(int i=0; i<num_fumadores; i++){
    hebra_fumador[i].join();
  }

  hebra_estanquero.join();
}
