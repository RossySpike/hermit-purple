## BACKEND

/backend
  /backend/includes
    /backend/includes/defines.h # fperror, bzero
    /backend/includes/files.h # struct for easy file handling 
    /backend/includes/hermit-purple.h # backend configuration
    /backend/includes/http-sanitize.h # validates params and headers
    /backend/includes/http-sanitize-custom-validators.h # custom validators provided by dev
    /backend/includes/server-defines.h # declare routes here
  /backend/src
    /backend/src/files.c
    /backend/src/server.c # server logic and main task, aiming it to be a state-machine
Left: thread, threads pools,..., api logic.

Maquina de estados
Si tu tarea es mayormente:

- I/O (leer disco, red) → hilos = 2× (núcleos CPU)
- CPU (convertir HEIC) → hilos = núcleos CPU
- Mixta → hilos = núcleos CPU + algo

                       ┌─────────────┐
                       │   COLA DE   │
                       │  TRABAJOS   │
                       │ (conexiones)│
                       └──────┬──────┘
                              │
              ┌───────────────┼───────────────┐
              │               │               │
              ▼               ▼               ▼
        ┌──────────┐   ┌──────────┐   ┌──────────┐
        │ Hilo 1   │   │ Hilo 2   │   │ Hilo 3   │
        │ espera   │   │ espera   │   │ espera   │
        └──────────┘   └──────────┘   └──────────┘
              │               │               │
              └───────────────┴───────────────┘
                              │
                        (todos esperan
                         en la misma cola)

Necesita:
1.cola de trabajos (thread-safe)
2.Inicializar el pool
3.funcion de hilo
4.agregar trabajo al hilo desde el principal
En este caso cada hilo hace la misma tarea

#### endpoints

GET /api/image/{id}?variant=thumbnail
GET /api/image/{id}?variant=compressed
GET /api/image/{id}?variant=original

POST /api/image
Content-Type: multipart/form-data
POST /api/image
Content-Type: multipart/form-data

Campos:

- file: [archivo HEIC/JPG/PNG]
- title: "Vacaciones" (opcional)
- description: "..." (opcional)

Response: 201 Created
Location: /api/image/nuevo_id

GET /api/image/cursor?current={cursor}&limit={limit}
GET /api/image/cursor?current=foto_123&limit=15

```json
Response: {
  "images": [
    {"id": "foto_124", "thumbnail": "/api/image/foto_124?variant=thumbnail", ...},
    ...
  ],
  "next_cursor": "foto_139",
  "has_more": true
}
```
