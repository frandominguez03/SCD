/*
* Práctica 2 - Sistemas Concurrentes y Distribuidos
* Problema del Barbero durmiente implementado con Monitores de Hoare
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
const int num_clientes = 5;
mutex mtx;

/* Plantilla para generar un entero aleatorio entre dos números, ambos incluidos */
template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max );
  return distribucion_uniforme( generador );
}

/* Función que simula la acción de cortarle el pelo a un cliente */
void cortarPeloACliente()
{
  chrono::milliseconds duracion_corte( aleatorio<50,500>() );

  mtx.lock();
  cout << "\t\tBarbero corta el pelo al cliente: ("
       << duracion_corte.count() << " milisegundos)..." << endl;
  mtx.unlock();

  this_thread::sleep_for(duracion_corte);

  mtx.lock();
  cout << "\t\tBarbero termina de pelar al cliente " << endl;
  mtx.unlock();
}

/* Función que simula que el cliente está esperando fuera de la barbería */
void EsperarFueraBarberia(int cliente)
{
  chrono::milliseconds duracion_espera( aleatorio<20,200>() );

  mtx.lock();
  cout << "\t\tCliente " << cliente << " espera fuera de la barbería durante ("
       << duracion_espera.count() << " milisegundos)..." << endl;
  mtx.unlock();

  this_thread::sleep_for(duracion_espera);
}

/* Clase del Monitor para controlar la barbería */
class Barberia : public HoareMonitor {
  private:
    int cliente;
    CondVar silla_vacia;
    CondVar esperando;
    CondVar cortando;

  public:
    Barberia();
    void cortarPelo(int cliente);
    void siguienteCliente();
    void finCliente();
};

/* Constructor por defecto */
Barberia::Barberia()
{
  silla_vacia = newCondVar();
  esperando = newCondVar();
  cortando = newCondVar();
}

/* Función cortarPelo(). Se manda una señal al barbero si está dormido,
o bloquea al cliente si ya hay uno pelándose */
void Barberia::cortarPelo(int i)
{

  mtx.lock();
  cout << "\t\tCliente " << i << " entra en la barbería" << endl;
  mtx.unlock();

  /* Si la silla está vacía, manda una señal al barbero */
  if(silla_vacia.get_nwt() == 0){
    silla_vacia.signal();
  }

  /* En otro caso, bloquea el cliente */
  else{
    esperando.wait();
  }

  mtx.lock();
  cout << "\t\tCliente " << i << " se está pelando" << endl;
  mtx.unlock();

  /* Bloquea el proceso porque el cliente se está pelando */
  cortando.wait();
}

/* Función siguienteCLiente. Pone el ingrediente i en el mostrador */
void Barberia::siguienteCliente()
{
  /* Si no hay clientes, espera hasta que llegue un cliente */
  if(esperando.get_nwt() == 0){
    silla_vacia.wait();
  }

  /* En caso contrario llama a uno de ellos */
  else{
    esperando.signal();
  }
}

void Barberia::finCliente()
{
  /* Manda una señal de que ha terminado con el cliente */
  cortando.signal();
}

/* Función hebra del cliente. En cada iteración del bucle infinito:
    - Llama al procedimiento del monitor cortarPelo(i), donde i es el número
    del cliente. Espera bloqueado hasta que termina de cortar el pelo al cliente
    - Llama a la función EsperarFueraBarberia(i), que hace que el cliente espere
    por un periodo aleatorio de tiempo */
void funcion_hebra_cliente(MRef<Barberia> monitor, int i)
{
  while(true)
  {
    monitor->cortarPelo(i);
    EsperarFueraBarberia(i);
  }
}

/* Función hebra del barbero. En cada iteración del bucle infinito:
    - Llama al procedimiento del monitor siguienteCliente(), que despierta al barbero y llama al 
    siguiente cliente
    - Llama a la función cortarPeloACliente(), indicando que le está cortando el pelo a este
    - Llama al procedimiento del monitor finCliente(), indicando que ya ha acabado de pelar
    al cliente */
void funcion_hebra_barbero(MRef<Barberia> monitor)
{
   while(true)
   {
     monitor->siguienteCliente();
     cortarPeloACliente();
     monitor->finCliente();
   }
}

int main()
{
  cout << endl << "--------------------------------------------------------" << endl
               << "Problema de la barbería" << endl
               << "--------------------------------------------------------" << endl
               << flush ;

  MRef<Barberia> monitor = Create<Barberia>();

  thread hebra_barbero(funcion_hebra_barbero);
  thread hebra_cliente[num_clientes];

  for(int i=0; i<num_clientes; i++){
    hebra_cliente[i] = thread(funcion_hebra_cliente, monitor, i);
  }

  for(int i=0; i<num_clientes; i++){
    hebra_cliente[i].join();
  }

  hebra_barbero.join();
}
