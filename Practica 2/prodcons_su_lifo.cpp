/*
* Práctica 2 - Sistemas Concurrentes y Distribuidos
* Problema de los productores y consumidores múltiples, versión LIFO
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

using namespace std ;
using namespace HM ;

/* Variables globales */
constexpr int
   num_items  = 40,        // número de items a producir/consumir
   num_productores = 8,
   num_consumidores = 5;

unsigned
   cont_prod[num_items] = {0}, // contadores de verificación: producidos
   cont_cons[num_items] = {0}; // contadores de verificación: consumidos

/* Plantilla para generar un entero aleatorio entre dos números, ambos incluidos */
template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

/* Función producir dato */
int producir_dato()
{
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   cout <<  "producido: " << contador << endl << flush ;
   cont_prod[contador]++; 
   return contador++;
}

/* Función consumir dato */
void consumir_dato( unsigned dato )
{
   if ( num_items <= dato )
   {
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   cout << "                  consumido: " << dato << endl ;
}

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

/* Clase del Monitor para ProdCons*/
class ProdCons1SU : public HoareMonitor {
 private:
 static const int           
   num_celdas_total = 10;   
 int
   buffer[num_celdas_total],
   primera_libre;
 CondVar libres;
 CondVar ocupadas;

 public:                    
   ProdCons1SU() ;           
   void  insertar(int dato);
   int extraer();
} ;

/* Constructor por defecto */
ProdCons1SU::ProdCons1SU()
{
   primera_libre   = 0;
   libres = newCondVar();
   ocupadas = newCondVar();
}

void ProdCons1SU::insertar(int dato)
{

   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   if ( primera_libre == num_celdas_total ){
      libres.wait();
   }

   buffer[primera_libre] = dato;
   primera_libre++;

   // señalar al productor que hay un hueco libre, por si está esperando
   ocupadas.signal();
}

int ProdCons1SU::extraer()
{
   if ( primera_libre == 0){
      ocupadas.wait();
   }

   primera_libre--;
   int valor = buffer[primera_libre];

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   libres.signal();

   return valor;
}


void funcion_hebra_productora( MRef<ProdCons1SU> monitor, int num_productores)
{
   for( unsigned i = 0 ; i < num_items/num_productores ; i++ )
   {
      int valor = producir_dato() ;
      monitor->insertar( valor );
   }
}

void funcion_hebra_consumidora( MRef<ProdCons1SU> monitor, int num_consumidores)
{
   for( unsigned i = 0 ; i < num_items/num_consumidores ; i++ )
   {
      int valor = monitor->extraer();
      consumir_dato(valor) ;
   }
}
// *****************************************************************************

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (Multiples prod/cons, Monitor SC, buffer LIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<ProdCons1SU> monitor = Create<ProdCons1SU>();

   thread hebra_productora[num_productores],
          hebra_consumidora[num_consumidores];

   for(int i=0; i<num_productores; i++){
      hebra_productora[i] = thread(funcion_hebra_productora, monitor, num_productores);
   }

   for(int i=0; i<num_consumidores; i++){
      hebra_consumidora[i] = thread(funcion_hebra_consumidora, monitor, num_consumidores);
   }

   for(int i=0; i<num_productores; i++){
      hebra_productora[i].join();
   }

   for(int i=0; i<num_consumidores; i++){
   	  hebra_consumidora[i].join();
   }

   /* Comprobaciones */
   test_contadores();
}
