/*
* Práctica 2 - Sistemas Concurrentes y Distribuidos
* Problema de los productores y consumidores múltiples, versión FIFO
* y señales SU (Señalar y Espera Urgente)
*
* autor: Francisco Domínguez Lorente
*/

#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

constexpr int
   n_items  = 40,
   num_hebras_prod=8,
   num_hebras_cons=5;
   
int contador=0, num_ocupadas=0;

mutex
   mtx;

unsigned
   cont_prod[n_items],
   cont_cons[n_items], 
   cont_num_prod[num_hebras_prod]={0}; 

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

int producir_dato(int indice)
{
  this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

  cout << "hebra numero: " << indice << "			produce: " << contador << endl << flush ;

  contador = indice * (n_items/num_hebras_prod) + cont_num_prod[indice];
  cont_num_prod[indice]++;
  cont_prod[contador]++;

  return contador;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   if ( n_items <= dato )
   {
      cout << " dato === " << dato << ", num_items == " << n_items << endl ;
      assert( dato < n_items );
   }

   cont_cons[dato]++;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   cout << "                  consumido: " << dato << endl ;
}
//----------------------------------------------------------------------

void ini_contadores()
{
   for( unsigned i = 0 ; i < n_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < n_items ; i++ )
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

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, un prod. y un cons.

class ProdCons2SU : public HoareMonitor
{
 private:
 static const int           // constantes:
   num_celdas_total = 10;   //  núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
   primera_libre,          //  indice de celda de la próxima inserción
   primera_ocupada;         

 CondVar ocupadas, libres;

 public:
   ProdCons2SU() ;
   int  leer();
   void escribir( int valor );
} ;

ProdCons2SU::ProdCons2SU()
{
   primera_libre = 0 ;
   primera_ocupada = 0 ;
   ocupadas=newCondVar();
   libres=newCondVar();
}

int ProdCons2SU::leer()
{

   if (num_ocupadas == 0){
      ocupadas.wait();
   }

   assert(0 <= primera_ocupada);
   const int valor = buffer[primera_ocupada];
   primera_ocupada=(primera_ocupada+1)%num_celdas_total;
   num_ocupadas--;

   libres.signal();

   return valor ;
}
// -----------------------------------------------------------------------------

void ProdCons2SU::escribir(int valor)
{

   if (num_ocupadas == num_celdas_total){
      libres.wait();
   }

   assert(primera_libre < num_celdas_total);

   buffer[primera_libre] = valor;
   primera_libre=(primera_libre+1)%num_celdas_total;
   num_ocupadas++;

   ocupadas.signal();

}
// *****************************************************************************
// funciones de hebras

// -----------------------------------------------------------------------------
void funcion_hebra_productora( MRef<ProdCons2SU> monitor, int indice)
{
   for( unsigned i = 0 ; i < n_items/num_hebras_prod; i++ )
   {
      int valor = producir_dato(indice) ;
      monitor->escribir(valor);
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdCons2SU> monitor )
{
   for( unsigned i = 0 ; i < n_items/num_hebras_cons ; i++ )
   {
      int valor = monitor->leer();
      consumir_dato(valor) ;
   }
}
// -----------------------------------------------------------------------------
int main()
{
    cout << "-------------------------------------------------------------------------------" << endl
          << "Problema de los productores-consumidores (1 prod/cons, Monitor SU, buffer FIFO). " << endl
          << "-------------------------------------------------------------------------------" << endl
          << flush ;

    MRef<ProdCons2SU> monitor = Create<ProdCons2SU>();

    thread hebra_productora[num_hebras_prod], hebra_consumidora[num_hebras_cons];

    for(int i=0; i<num_hebras_prod; i++){
      hebra_productora[i]=thread(funcion_hebra_productora, monitor, i);      
    }

    for(int i=0; i<num_hebras_cons; i++){     
      hebra_consumidora[i]=thread(funcion_hebra_consumidora, monitor);
    }    

    for(int i=0; i<num_hebras_prod; i++){
      hebra_productora[i].join() ;
    }
    
    for(int i=0; i<num_hebras_cons; i++){
      hebra_consumidora[i].join() ;
    }    

   /* Comprobaciones */
   test_contadores() ;
}