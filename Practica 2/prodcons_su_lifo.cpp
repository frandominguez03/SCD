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
   num_items  = 40;        // número de items a producir/consumir

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
int producir_dato(int indice)
{
   static int contador = {0} ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   cout << "hebra productora: " << indice << ", produce: " << contador << endl << flush ;
   cont_prod[indice]++; 
   return contador;
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

void ini_contadores()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
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
 static const int           // constantes:
   num_celdas_total = 10;   //  núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
   cima;          //  indice de celda de la próxima inserción
 CondVar libres;
 CondVar ocupadas;

 public:                    // constructor y métodos públicos
   ProdCons1SU() ;           // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdCons1SU::ProdCons1SU()
{
   cima   = 0;
   libres = newCondVar();
   ocupadas = newCondVar();
}
// -----------------------------------------------------------------------------

int ProdCons1SU::leer(  )
{

   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   if ( cima == 0 ){
      libres.wait();
   }

   cima-- ;
   const int valor = buffer[cima] ;


   // señalar al productor que hay un hueco libre, por si está esperando
   ocupadas.signal();

   // devolver valor
   return valor ;
}

void ProdCons1SU::escribir( int valor )
{
   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   if ( cima == num_celdas_total){
      ocupadas.wait();
   }

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[cima] = valor ;
   cima++ ;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   libres.signal();
}

// -----------------------------------------------------------------------------

void funcion_hebra_productora( MRef<ProdCons1SU> monitor, int num_hebra)
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      int valor = producir_dato(num_hebra) ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdCons1SU> monitor)
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      int valor = monitor->leer();
      consumir_dato( valor ) ;
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

   thread hebra_productora[num_items],
          hebra_consumidora[num_items];

   for(int i=0; i<num_items; i++){
      hebra_productora[i] = thread(funcion_hebra_productora, monitor, i);
   }

   for(int i=0; i<num_items; i++){
      hebra_consumidora[i] = thread(funcion_hebra_consumidora, monitor);
   }

   for(int i=0; i<num_items; i++){
      hebra_productora[i].join();
      hebra_consumidora[i].join();
   }

   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
