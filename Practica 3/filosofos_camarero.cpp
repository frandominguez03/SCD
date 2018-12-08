/*
* Práctica 3 - Sistemas Concurrentes y Distribuidos
* Problema de los filósofos con camarero implementado con algoritmos distribuidos con MPI
* 
* Para compilar:
* mpic++ -std=c++11 -c filosofos_camarero.cpp
* mpic++ -std=c++11 -o filosofos_camarero filosofos_camarero.o
* mpirun -np 10 ./filosofos_camarero
*
* autor: Francisco Domínguez Lorente
*/


#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_filosofos = 5 ,
   num_procesos  = 2*num_filosofos + 1, // Dos por cada filósofo, más el camarero
   id_camarero = 10,
   etiq_soltar_tenedor = 0,
   etiq_coger_tenedor = 1,
   etiq_sentarse = 2,
   etiq_levantarse = 3 ;


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

// ---------------------------------------------------------------------

void funcion_filosofos( int id )
{
  int id_ten_izq = (id+1)              % num_procesos, //id. tenedor izq.
      id_ten_der = (id+num_procesos-1) % num_procesos; //id. tenedor der.
  int valor = 1;  // De donde se leen los datos
  MPI_Status estado;

  while ( true )
  {
    // Primero solicita sentarse
    cout << "Filósofo " << id << " solicita asiento" << endl;
    MPI_Ssend(&valor, 1, MPI_INT, id_camarero, etiq_sentarse, MPI_COMM_WORLD);

    // Espera a que el camarero le ceda un asiento
    MPI_Recv(&valor, 1, MPI_INT, id_camarero, etiq_sentarse, MPI_COMM_WORLD, &estado);

    // Solicita el tenedor izquierdo
    cout << "Filósofo " << id << " solicita ten. izq." << id_ten_izq << endl;
    MPI_Ssend(&valor, 1, MPI_INT, id_ten_izq, etiq_coger_tenedor, MPI_COMM_WORLD);

    // Solicita el tenedor derecho
    cout << "Filósofo " << id <<" solicita ten. der." << id_ten_der << endl;
    MPI_Ssend(&valor, 1, MPI_INT, id_ten_der, etiq_coger_tenedor, MPI_COMM_WORLD);

    // Comiendo
    cout << "Filósofo " << id <<" comienza a comer" << endl ;
    sleep_for( milliseconds( aleatorio<10,100>() ) );

    // Suelta el tenedor izquierdo
    cout << "Filósofo " << id <<" suelta ten. izq. " << id_ten_izq << endl;
    MPI_Ssend(&valor, 1, MPI_INT, id_ten_izq, etiq_soltar_tenedor, MPI_COMM_WORLD);

    // Suelta el tenedor derecho
    cout<< "Filósofo " << id <<" suelta ten. der. " << id_ten_der <<endl;
    MPI_Ssend(&valor, 1, MPI_INT, id_ten_der, etiq_soltar_tenedor, MPI_COMM_WORLD);

    // Manda una señal al camarero de que se levanta
    cout << "Filósofo " << id << " se levanta" << endl;
    MPI_Ssend(&valor, 1, MPI_INT, id_camarero, etiq_levantarse, MPI_COMM_WORLD);

    // Pensando
    cout << "Filosofo " << id << " comienza a pensar" << endl;
    sleep_for( milliseconds( aleatorio<10,100>() ) );
 }
}
// ---------------------------------------------------------------------

void funcion_tenedores( int id )
{
  int valor, id_filosofo ;  // valor recibido, identificador del filósofo
  MPI_Status estado ;       // metadatos de las dos recepciones

  while ( true )
  {
     MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_coger_tenedor, MPI_COMM_WORLD, &estado);
     id_filosofo = estado.MPI_SOURCE;
     cout << "Ten. " << id << " ha sido cogido por filo. " << id_filosofo << endl;

     MPI_Recv(&valor, 1, MPI_INT, id_filosofo, etiq_soltar_tenedor, MPI_COMM_WORLD, &estado);
     cout << "Ten. "<< id << " ha sido liberado por filo. " << id_filosofo << endl ;
  }
}

void funcion_camarero()
{
  int num_fils=0, valor, id_filosofo;
  MPI_Status estado;

  while(true){
    // Inicializamos la etiqueta del estado a 0
    estado.MPI_TAG = 0;

    // Si el número de filósofos sentados es menor que 4, se pueden sentar o levantar
    if(num_fils < 4){
      MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado);
    }

    // En caso contrario solo se pueden levantar
    else{
      MPI_Probe(MPI_ANY_SOURCE, etiq_levantarse, MPI_COMM_WORLD, &estado);
    }

    // Si la etiqueta coincide con la petición de sentarse, procede
    if(estado.MPI_TAG == etiq_sentarse){
      // Igualamos el filósofo actual al proceso que hace la petición
      id_filosofo = estado.MPI_SOURCE;

      // Recibimos la petición de sentarse del filósofo
      MPI_Recv(&valor, 1, MPI_INT, id_filosofo, etiq_sentarse, MPI_COMM_WORLD, &estado);

      // Aumentamos el número de filósofos que hay sentados
      num_fils++;

      // Enviamos el mensaje de que se puede sentar e imprimimos por pantalla
      MPI_Ssend(&valor, 1, MPI_INT, id_filosofo, etiq_sentarse, MPI_COMM_WORLD);
      cout << "Se ha sentado el filosofo "<< id_filosofo << " . Hay " << num_fils << " filosofos sentados." << endl;
    }

    // Si la etiqueta coincide con la petición de levantarse, procede
    if(estado.MPI_TAG == etiq_levantarse){
      // Igualamos el filósofo actual al proceso que hace la petición
      id_filosofo = estado.MPI_SOURCE;

      // Recibimos la petición de sentarse del filósofo
      MPI_Recv(&valor, 1, MPI_INT, id_filosofo, etiq_levantarse, MPI_COMM_WORLD, &estado);

      // Se levanta el filósofo
      num_fils--;
      cout << "Se ha levantado el filosofo "<< id_filosofo << " . Hay " << num_fils << " filosofos sentados." << endl;
    }
  }
}
// ---------------------------------------------------------------------

int main( int argc, char** argv )
{
   int id_propio, num_procesos_actual ;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


   if ( num_procesos == num_procesos_actual )
   {
      // ejecutar la función correspondiente a 'id_propio'
      if( id_propio == id_camarero)      // Identificador del camarero
         funcion_camarero();
      else if ( id_propio % 2 == 0 )     // si es par
         funcion_filosofos( id_propio ); //   es un filósofo
      else                               // si es impar
         funcion_tenedores( id_propio ); //   es un tenedor
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   MPI_Finalize( );
   return 0;
}

// ---------------------------------------------------------------------