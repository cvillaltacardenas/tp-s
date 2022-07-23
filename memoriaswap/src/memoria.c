#include "include/swap.h"
#include "include/memoria.h"
#include <math.h>

t_log* logger;
t_config * config;

// Variables globales
t_config* config;
int memoria_fd;
int cliente_fd;
char* ipMemoria, *puertoMemoria;
int entradas_por_tabla, tamanio_memoria, tamanio_pagina;

t_list* lista_registros_primer_nivel;
t_list* lista_registros_segundo_nivel;
t_list* lista_tablas_primer_nivel;
t_list* lista_tablas_segundo_nivel;
void* espacio_usuario_memoria;
t_bitarray	* frames_disponibles;
void* bloque_frames_lilbres;


void crear_espacio_usuario();

int main(void) {
	//sem_t semMemoria;

	config = iniciarConfig(CONFIG_FILE);
	logger = iniciarLogger("memoria.log", "Memoria");
	ipMemoria= config_get_string_value(config,"IP_MEMORIA");
    puertoMemoria= config_get_string_value(config,"PUERTO_ESCUCHA");
    entradas_por_tabla = config_get_int_value(config,"ENTRADAS_POR_TABLA");
    tamanio_memoria = config_get_int_value(config,"TAM_MEMORIA");
    tamanio_pagina = config_get_int_value(config,"TAM_PAGINA");
    preparar_modulo_swap();
    iniciar_estructuras_administrativas_kernel();
    crear_espacio_usuario();
	//sem_init(&semMemoria, 0, 1);

	// Inicio el servidor
	memoria_fd = iniciarServidor(ipMemoria, puertoMemoria, logger);
	log_info(logger, "Memoria lista para recibir a Kernel o CPU");

    while (1) {
        escuchar_cliente("CPU");
        escuchar_cliente("KERNEL");
    }

	return EXIT_SUCCESS;
}

void procesar_conexion(void* void_args) {

    t_procesar_conexion_attrs* attrs = (t_procesar_conexion_attrs*) void_args;
	t_log* logger = attrs->log;
    int cliente_fd = attrs->fd;
    handshake_cpu_memoria(cliente_fd, tamanio_pagina, entradas_por_tabla, HANDSHAKE_MEMORIA);
    free(attrs);

      while(cliente_fd != -1) {
	        op_code cod_op = recibirOperacion(cliente_fd);
            switch (cod_op) {
                case MENSAJE:
                    recibirMensaje(cliente_fd, logger);
                    break;
                case ESCRIBIR_MEMORIA:
                    enviarMensaje("Voy a escribir en memoria...", cliente_fd);
                    // sem_wait(&semMemoria);
                    // Escribir en memoria...
                    // sem_signal(&semMemoria);
                    enviarMensaje("Ya escribí en memoria!", cliente_fd);
                    break;
                case LEER_MEMORIA:
                    enviarMensaje("Voy a leer la memoria...", cliente_fd);
                    // sem_wait(&semMemoria);
                    // Leer memoria...
                    // sem_signal(&semMemoria);
                    enviarMensaje("Ya leí la memoria!", cliente_fd);
                    break;
                case SWAPEAR_PROCESO:
                	log_info(logger, "Recibi un PCB a swapear");
                	t_pcb* pcb = recibirPCB(cliente_fd);
                	printf("PID: %zud\n",pcb->idProceso);
                	swapear_proceso(pcb);
                	break;
                case CREAR_ESTRUCTURAS_ADMIN:
                    ;
                    t_pcb* pcb_kernel = recibirPCB(cliente_fd);
                    crear_archivo_swap(pcb_kernel->idProceso, pcb_kernel->tamanioProceso);
                    pcb_kernel->tablaPaginas = crear_estructuras_administrativas_proceso(pcb_kernel->tamanioProceso) - 1;
                    enviarPCB(cliente_fd,pcb_kernel, ACTUALIZAR_INDICE_TABLA_PAGINAS);
                    break;
                case OBTENER_ENTRADA_SEGUNDO_NIVEL:
                    printf("\nMEMORIA entró en OBTENER_ENTRADA_SEGUNDO_NIVEL\n");
                    void* buffer_tabla_segundo_nivel = recibirBuffer(cliente_fd);

                    size_t nro_tabla_primer_nivel;
                    uint32_t entrada_tabla_primer_nivel;
                    memcpy(&nro_tabla_primer_nivel, buffer_tabla_segundo_nivel, sizeof(size_t));
                    memcpy(&entrada_tabla_primer_nivel, buffer_tabla_segundo_nivel + sizeof(size_t), sizeof(uint32_t));
                    printf("\ntabla_primer_nivel: %zu\n", nro_tabla_primer_nivel);
                    printf("\nentrada_tabla_primer_nivel: %d\n", entrada_tabla_primer_nivel);
                    t_registro_primer_nivel* registro_primer_nivel = (list_get(list_get(lista_tablas_primer_nivel, nro_tabla_primer_nivel), entrada_tabla_primer_nivel)) ;
                    uint32_t nro_tabla_segundo_nivel = registro_primer_nivel->nro_tabla_segundo_nivel;
                    enviar_entero(cliente_fd, nro_tabla_segundo_nivel, OBTENER_ENTRADA_SEGUNDO_NIVEL);
                    break;
                case OBTENER_MARCO:
                    printf("\nMEMORIA entró en OBTENER_MARCO\n");
                    void* buffer_marco = recibirBuffer(cliente_fd);

                    uint32_t entrada_tabla_segundo_nivel;
                    memcpy(&nro_tabla_segundo_nivel, buffer_marco, sizeof(uint32_t));
                    memcpy(&entrada_tabla_segundo_nivel, buffer_marco+sizeof(uint32_t), sizeof(uint32_t));
                    printf("\nnro de tabla de segundo nivel es: %zu\n", nro_tabla_segundo_nivel);
                    printf("\nla entrada en la tabla de segundo nivel es: %zu\n", entrada_tabla_segundo_nivel);
                    t_registro_segundo_nivel* registro_segundo_nivel = list_get(list_get(lista_tablas_segundo_nivel, nro_tabla_segundo_nivel), entrada_tabla_segundo_nivel);
                    uint32_t marco = 84; //registro_segundo_nivel->frame;
                    enviar_entero(cliente_fd, marco, OBTENER_MARCO);
                    break;
                case -1:
                    log_info(logger, "El cliente se desconectó");
                    cliente_fd = -1;
                    break;
                default:
                    log_warning(logger, "Operacion desconocida.");
                    break;
            }

	}
}

int escuchar_cliente(char *nombre_cliente) {
    int cliente = esperarCliente(memoria_fd, logger);
    if (cliente != -1) {
        pthread_t hilo;
        t_procesar_conexion_attrs* attrs = malloc(sizeof(t_procesar_conexion_attrs));
        attrs->log = logger;
        attrs->fd = cliente;
        pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) attrs);
        pthread_detach(hilo);
        return 1;
    }
    return 0;
}

void preparar_modulo_swap(){
	cola_swap = queue_create();
	PATH_SWAP = config_get_string_value(config,"PATH_SWAP");
	string_append(&PATH_SWAP,"/");
	RETARDO_SWAP = config_get_int_value(config,"RETARDO_SWAP");
}

size_t crear_estructuras_administrativas_proceso(size_t tamanio_proceso) {

    int cantidad_entradas_tabla_segundo_nivel = MAX((tamanio_proceso/tamanio_pagina),1);
    int indice_primer_nivel = 0;
    int indice_segundo_nivel=0;

    t_registro_segundo_nivel* registro_tabla_segundo_nivel;
    t_registro_primer_nivel* registro_tabla_primer_nivel;

    while(indice_segundo_nivel < cantidad_entradas_tabla_segundo_nivel) {
        registro_tabla_primer_nivel = malloc(sizeof(t_registro_primer_nivel));
        registro_tabla_primer_nivel->indice = indice_primer_nivel;
        registro_tabla_primer_nivel->nro_tabla_segundo_nivel = list_size(lista_tablas_segundo_nivel);
        list_add(lista_registros_primer_nivel, registro_tabla_primer_nivel);

        for (int j=0; j<entradas_por_tabla; j++){
            registro_tabla_segundo_nivel = malloc(sizeof(t_registro_segundo_nivel));
            registro_tabla_segundo_nivel->indice = j;
            registro_tabla_segundo_nivel->frame = 0;
            registro_tabla_segundo_nivel->modificado=false;
            registro_tabla_segundo_nivel->usado=false;
            registro_tabla_segundo_nivel->presencia=false;

            list_add(lista_registros_segundo_nivel, registro_tabla_segundo_nivel);
            indice_segundo_nivel++;
        }
        indice_primer_nivel++;

        list_add(lista_tablas_segundo_nivel, lista_registros_segundo_nivel);
       // list_clean(lista_registros_segundo_nivel);
    }
    list_add(lista_tablas_primer_nivel, lista_registros_primer_nivel);
    return list_size(lista_tablas_primer_nivel);
}

void crear_bitmap_frames_libres() {
    uint32_t tamanio_bit_array = tamanio_memoria / tamanio_pagina;
    bloque_frames_lilbres = malloc(tamanio_bit_array);
    frames_disponibles = bitarray_create_with_mode(bloque_frames_lilbres, tamanio_bit_array, LSB_FIRST);
}

void iniciar_estructuras_administrativas_kernel() {
    lista_registros_primer_nivel = list_create();
    lista_registros_segundo_nivel = list_create();
    lista_tablas_primer_nivel = list_create();
    lista_tablas_segundo_nivel = list_create();
    crear_bitmap_frames_libres();

}

void crear_espacio_usuario() {
    espacio_usuario_memoria = malloc(tamanio_memoria);
}

int tiene_marcos_disponibles() {

}



