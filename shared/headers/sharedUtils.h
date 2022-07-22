#ifndef TP_2022_1C_ECLIPSO_SHAREDUTILS_H
#define TP_2022_1C_ECLIPSO_SHAREDUTILS_H

#include <stdbool.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include<stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include<errno.h>
#include<commons/config.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/string.h>
#include <pthread.h>

extern int errno;

typedef uint32_t operando;

typedef enum {
    MENSAJE,
    LISTA_INSTRUCCIONES,
    PCB,
    TERMINAR_PROCESO,
	BLOQUEAR_PROCESO,
	CONTINUA_PROCESO,
	DESALOJAR_PROCESO,
    ESCRIBIR_MEMORIA,
    LEER_MEMORIA,
	SWAPEAR_PROCESO,
	CREAR_ESTRUCTURAS_ADMIN,
	ACTUALIZAR_INDICE_TABLA_PAGINAS
} op_code;

typedef enum
{
    NO_OP,
    IO,
    READ,
    COPY,
    WRITE,
    EXIT
} instr_code;

typedef struct{
    instr_code codigo_operacion;
    operando parametros[2];
} t_instruccion;

typedef struct
{
    size_t size;
    void* stream;
} t_buffer;

typedef struct
{
    op_code codigo_operacion;
    t_buffer* buffer;
} t_paquete;

typedef struct {
    size_t idProceso;
    size_t tamanioProceso;
    size_t programCounter;
    size_t tablaPaginas;
    size_t estimacionRafaga;
    t_list* listaInstrucciones;
    size_t kernel_fd;
    size_t consola_fd;
} t_pcb;

typedef struct
{
	t_pcb* pcb;
	int conexion_cpu_dispatch;
	int conexion_cpu_interrupt;
	int conexion_memoria;
	char* algoritmo_planificacion;
	t_log* logger;
	uint32_t tiempo_maximo_bloqueado;
	double alpha;
} t_attrs_planificacion;

typedef struct
{
	uint32_t entrada_tabla_primer_nivel;
	uint32_t entrada_tabla_segundo_nivel;
	uint32_t offset;
} dir_logica;

typedef struct
{
	uint32_t marco;
	uint32_t offset;
} dir_fisica;

t_config* iniciarConfig(char*);
t_log* iniciarLogger(char*, char*);
t_paquete* crearPaquete(void);
void crearBuffer(t_paquete *);
void* serializarPaquete(t_paquete *, size_t );
void eliminarPaquete(t_paquete *);
int enviarPaquete(t_paquete*, int);
void enviarMensaje(char*, int);
op_code recibirOperacion(int);
void recibirMensaje(int, t_log*);
void liberarConexion(int);
void terminarPrograma(uint32_t, t_log*, t_config*);
void* recibirBuffer(int);
void verificarListen(int);
void verificarBind(int, struct addrinfo*);
void verificarConnect(int, struct sockaddr_in *);
int crearConexion(char*, int, char*);
int iniciarServidor(char*, char*, t_log*);
int esperarCliente(int, t_log*);
void agregarInstruccion(t_paquete*, void*);
void agregarListaInstrucciones(t_paquete *, t_list *);
void agregarTamanioProceso(t_paquete*, int);
void agregarEntero(t_paquete *, size_t);
void enviarPCB(int, t_pcb*, op_code);
void enviar_interrupcion(int, op_code);
t_pcb* recibirPCB(int);
t_list* deserializarListaInstrucciones(void* , size_t , t_list*);
void enviar_interrupcion(int, op_code);
void logear_PCB(t_log* logger,t_pcb* pcb,char* enviado_recibido);

#endif //TP_2022_1C_ECLIPSO_SHAREDUTILS_H
