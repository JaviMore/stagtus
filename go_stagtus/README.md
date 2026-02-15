# Stagtus - System Tray Status App

Aplicación de bandeja del sistema para controlar estados (Coffee Time, Busy Mode, Meeting).

## Requisitos

- Go 1.25.6 o superior
- Windows 10/11 (64-bit)
- CGO habilitado (verificar con `go env CGO_ENABLED`)

## Compilación

### Primera vez (instalar herramienta de recursos)

```powershell
go install github.com/akavel/rsrc@latest
```

### Compilar el ejecutable con icono

```powershell
# 1. Generar recursos de Windows con el icono
rsrc -ico stagtus.ico -o rsrc.syso

# 2. Compilar el ejecutable
go build -ldflags="-H=windowsgui -s -w" -o stagtus.exe
```

### Parámetros de compilación

- `-H=windowsgui`: Aplicación GUI sin consola
- `-s -w`: Reduce tamaño del ejecutable (elimina símbolos de debug)

## Ejecución

```powershell
.\stagtus.exe
```

La aplicación se ejecuta en la bandeja del sistema (System Tray) sin ventana visible.

## Estructura del proyecto

```
stagtus/
├── stagtus.go          # Código principal
├── stagtus.ico         # Icono del ejecutable
├── go.mod              # Dependencias
├── FyneApp.toml        # Configuración Fyne
├── images/             # Iconos para la bandeja
│   ├── coffee.png
│   ├── working.png
│   └── phone.png
└── README.md
```

## Notas

- El archivo `rsrc.syso` se genera automáticamente y contiene el icono embebido
- No usar `fyne package` - genera ejecutables incompatibles con Go 1.25+
- Si cambias el icono, debes regenerar `rsrc.syso` antes de compilar
