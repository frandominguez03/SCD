/* @author: Francisco Domínguez Lorente */

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

const int MAX = 3;
Semaphore mostr_vacio = 1;
Semaphore ingr_disp[MAX] = {0,0,0}; // Crear array de semaforos
mutex mtx;

// Hay 2 semanas para hacer esta practica, a la semana siguiente hay un examen ()
// En la quinta sesion haremos un ejercicio similar con semaforos. 1h/1h30min para hacerlo.


//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
	int ingrediente;
	// Calculamos los milisegundos aleatorios para producir un elemento
	chrono::milliseconds duracion_producir( aleatorio<20,200>() );

	// Bloqueamos con el valor obtenido anteriormente
	this_thread::sleep_for(duracion_producir);

	while(true){
		sem_wait(mostr_vacio);
		ingrediente = aleatorio<0,2>();
		mtx.lock();
		cout << "Puesto el ingrediente: " << ingrediente << endl;
		mtx.unlock();
		sem_signal(ingr_disp[ingrediente]);
	}
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{
	mtx.lock();
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,2000>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador:" << num_fumador << " termina de fumar, comienza espera de ingrediente." << endl;

    mtx.unlock();
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while(true)
   {
   		sem_wait(ingr_disp[num_fumador]);
   		mtx.lock();
   		cout << "Retirado el ingrediente: " << num_fumador << endl;
   		mtx.unlock();
   		sem_signal(mostr_vacio);
   		fumar(num_fumador);
   }
}

//----------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------" << endl
        << "Problema de los fumadores" << endl
        << "--------------------------------------------------------" << endl
        << flush ;

    thread hebra_estanquero(funcion_hebra_estanquero);
    thread hebra_fumador[MAX];

    for(int i=0; i<MAX; i++){
    	hebra_fumador[i] = thread(funcion_hebra_fumador, i);
    }

    for(int i=0; i<MAX; i++){
    	hebra_fumador[i].join();
    }

  	hebra_estanquero.join();    
}
