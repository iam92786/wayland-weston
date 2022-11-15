#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
 
#include <wayland-client.h>
 
typedef uint32_t pixel;
 
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
 
struct wl_compositor *compositor;
struct wl_display *display;
struct wl_pointer *pointer;
struct wl_seat *seat;
struct wl_shell *shell;
struct wl_shm *shm;
 
struct wl_registry_listener registry_listener;
struct wl_pointer_listener pointer_listener;
 
void setup_wayland(void)
{
	struct wl_registry *registry;
 
	display = wl_display_connect(NULL);
	if (display == NULL) {
		perror("Error : Connecting display");
		exit(EXIT_FAILURE);
	}
	registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display);
	wl_registry_destroy(registry);
}
 
void cleanup_wayland(void)
{
	wl_seat_destroy(seat);
	wl_shell_destroy(shell);
	wl_shm_destroy(shm);
	wl_compositor_destroy(compositor);
	wl_display_disconnect(display);
}
 
void registry_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
	if (strcmp(interface, wl_compositor_interface.name) == 0){
		compositor = wl_registry_bind(registry, name, &wl_compositor_interface, min(version, 4));
		printf("p1: %s\n", interface);
	}
	else if (strcmp(interface, wl_shm_interface.name) == 0){
		shm = wl_registry_bind(registry, name, &wl_shm_interface, min(version, 1));
		printf("p2: %s\n", interface);
	}
	else if (strcmp(interface, wl_shell_interface.name) == 0){
		shell = wl_registry_bind(registry, name, &wl_shell_interface, min(version, 1));
		printf("p3: %s\n", interface);
	}
	else if (strcmp(interface, wl_seat_interface.name) == 0) {
		seat = wl_registry_bind(registry, name, &wl_seat_interface, min(version, 2));
		printf("p4: %s\n", interface);
	}
}
 
void registry_global_remove(void *a, struct wl_registry *b, uint32_t c)
{ 
}
 
struct wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove
};
 
struct pool_data {
	int fd;
	pixel *memory;
	unsigned capacity;
	unsigned size;
};
 
struct wl_shm_pool *create_memory_pool(int file)
{
	struct pool_data *data;
	struct wl_shm_pool *pool;
	struct stat stat;
 
	if (fstat(file, &stat) != 0) return NULL;
 
	data = malloc(sizeof(struct pool_data));
	if (data == NULL) 
        return NULL;
 
	data->capacity = stat.st_size;
	data->size = 0;
	data->fd = file;
 
	data->memory = mmap(0, data->capacity,PROT_READ, MAP_SHARED, data->fd, 0);
	if (data->memory == MAP_FAILED) 
        goto cleanup_alloc;
 
	pool = wl_shm_create_pool(shm, data->fd, data->capacity);
	if (pool == NULL) 
        goto cleanup_mmap;
 
	wl_shm_pool_set_user_data(pool, data);

	return pool;
 
cleanup_mmap:
	munmap(data->memory, data->capacity);

cleanup_alloc:
	free(data);
	return NULL;
}
 
void free_memory_pool(struct wl_shm_pool *pool)
{
	struct pool_data *data;
 
	data = wl_shm_pool_get_user_data(pool);
	wl_shm_pool_destroy(pool);
	munmap(data->memory, data->capacity);
	free(data);
}
 
uint32_t PIXEL_FORMAT_ID = WL_SHM_FORMAT_ARGB8888;
//uint32_t PIXEL_FORMAT_ID =  WL_SHM_FORMAT_RGB888;
 
struct wl_buffer *create_buffer(struct wl_shm_pool *pool, unsigned width, unsigned height)
{
	struct pool_data *pool_data;
	struct wl_buffer *buffer;
 
	pool_data = wl_shm_pool_get_user_data(pool);
	buffer = wl_shm_pool_create_buffer(pool, pool_data->size, width, height, width*sizeof(pixel), PIXEL_FORMAT_ID);
    //wl_buffer*wl_shm_pool_create_buffer(wl_shm_pool* wl_shm_pool_int offset,int width,int height,int stride,uint format)
	if (buffer == NULL) 
        return NULL;
 
	pool_data->size += width*height*sizeof(pixel);
 
	return buffer;
}
 
void shell_surface_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial)
{
	wl_shell_surface_pong(shell_surface, serial);
}
 
void shell_surface_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height)
{ 
}
 
struct wl_shell_surface_listener shell_surface_listener = {
	.ping = shell_surface_ping,
	.configure = shell_surface_configure,
};
 
struct wl_shell_surface *create_surface(void)
{
	struct wl_surface *surface;
	struct wl_shell_surface *shell_surface;
 
	surface = wl_compositor_create_surface(compositor);
 
	if (surface == NULL) return NULL;
 
	shell_surface = wl_shell_get_shell_surface(shell, surface);
 
	if (shell_surface == NULL) {
		wl_surface_destroy(surface);
		return NULL;
	}
 
	wl_shell_surface_add_listener(shell_surface, &shell_surface_listener, 0);
	wl_shell_surface_set_toplevel(shell_surface);
	wl_shell_surface_set_user_data(shell_surface, surface);
	wl_surface_set_user_data(surface, NULL);
 
	return shell_surface;
}
 
void free_surface(struct wl_shell_surface *shell_surface)
{
	struct wl_surface *surface;
 
	surface = wl_shell_surface_get_user_data(shell_surface);
	wl_shell_surface_destroy(shell_surface);
	wl_surface_destroy(surface);
}
 
void bind_buffer(struct wl_buffer *buffer, struct wl_shell_surface *shell_surface)
{
	struct wl_surface *surface;
 
	surface = wl_shell_surface_get_user_data(shell_surface);
	wl_surface_attach(surface, buffer, 0, 0);
	wl_surface_commit(surface);
}
 
int main(void)
{
	struct wl_buffer *buffer;
	struct wl_shm_pool *pool;
	struct wl_shell_surface *surface;
	int image;
 
	setup_wayland();
    
	image = open("bird-640.bmp", O_RDWR);
 
	if (image < 0) {
	    perror("Error: Opening surface image");
	    return EXIT_FAILURE;
	}
 
	pool = create_memory_pool(image);
	surface = create_surface();
	buffer = create_buffer(pool,640,445);
	bind_buffer(buffer, surface);
	
	wl_display_dispatch(display);
	
	sleep(5);
	
	wl_buffer_destroy(buffer);
	
	free_surface(surface);
	free_memory_pool(pool);
	close(image);
	cleanup_wayland();
 
	return EXIT_SUCCESS;
}



/*
REF : https://decode.red/net/archives/392   
*/