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
   assert( dato < num_items );
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
class ProdCons2SU : public HoareMonitor {
 private:
 static const int           
   num_celdas_total = 10;   
 int
   buffer[num_celdas_total],
   primera_libre,
   primera_ocupada;
 CondVar libres;
 CondVar ocupadas;

 public:                    
   ProdCons2SU() ;           
   void  insertar(int dato);
   int extraer();
} ;

/* Constructor por defecto */
ProdCons2SU::ProdCons2SU()
{
   primera_libre   = 0;
   primera_ocupada = 0;
   libres = newCondVar();
   ocupadas = newCondVar();
}

void ProdCons2SU::insertar(int dato)
{

   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   if ( primera_libre == num_celdas_total ){
      libres.wait();
   }

   buffer[primera_libre] = dato;
   ocupadas.signal();
   primera_libre = primera_libre + (primera_libre%num_celdas_total);
}

int ProdCons2SU::extraer()
{
   if ( primera_libre == 0){
      ocupadas.wait();
   }

   int valor = buffer[primera_ocupada];
   libres.signal();
   primera_ocupada = primera_ocupada + (primera_ocupada%num_celdas_total);

   return valor;
}


void funcion_hebra_productora( MRef<ProdCons2SU> monitor, int num_productores)
{
   for( unsigned i = 0 ; i < num_items/num_productores ; i++ )
   {
      int valor = producir_dato() ;
      monitor->insertar( valor );
   }
}

void funcion_hebra_consumidora( MRef<ProdCons2SU> monitor, int num_consumidores)
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
        << "Problema de los productores-consumidores (Multiples prod/cons, Monitor SC, buffer FIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<ProdCons2SU> monitor = Create<ProdCons2SU>();

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

   test_contadores();
   /* Comprobaciones */
   
}