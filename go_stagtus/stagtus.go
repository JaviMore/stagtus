package main

import (
	"embed"
	"fmt"
	"log"

	"fyne.io/fyne/v2"
	"fyne.io/fyne/v2/app"
	"fyne.io/fyne/v2/driver/desktop"
	"go.bug.st/serial"
)

// 1. EMBEDDING
// Metemos las imágenes dentro del ejecutable automáticamente
//
//go:embed images/*.png
var content embed.FS

// Configuración Puerto
const portName = "COM13"
const baudRate = 115200

var serialPort serial.Port

func main() {
	// Iniciamos la app (necesaria aunque no mostremos ventana)
	a := app.New()

	// Forzamos que la app no se cierre al cerrar ventanas (aunque no tenemos)
	a.SetIcon(loadResource("images/coffee.png", "icon"))

	// Verificamos soporte de escritorio
	var desk desktop.App
	if d, ok := a.(desktop.App); ok {
		desk = d
	} else {
		log.Fatal("Tu sistema no soporta System Tray")
	}

	// 2. CARGAR RECURSOS
	// Usaremos las mismas imágenes para el icono del reloj Y para el menú
	iconCoffee := loadResource("images/coffee.png", "coffee")
	iconBusy := loadResource("images/working.png", "busy")
	iconMeeting := loadResource("images/phone.png", "meeting")

	// 3. CREAR ITEMS DEL MENÚ
	// Creamos los items pero NO definimos la acción todavía para poder referenciarlos entre ellos
	itemCoffee := fyne.NewMenuItem("Coffee Time", nil)
	itemBusy := fyne.NewMenuItem("Busy Mode", nil)
	itemMeeting := fyne.NewMenuItem("Meeting", nil)

	// --- AQUI ESTÁ LA CLAVE: ASIGNAR ICONOS AL MENÚ ---
	itemCoffee.Icon = iconCoffee
	itemBusy.Icon = iconBusy
	itemMeeting.Icon = iconMeeting

	// Crear item Quit explícito
	itemQuit := fyne.NewMenuItem("Quit", func() {
		a.Quit()
	})

	// 4. DEFINIR LA LÓGICA (ACCIONES)
	// Función helper para actualizar todo de golpe
	updateState := func(activeItem *fyne.MenuItem, activeIcon fyne.Resource, cmd string) {
		// 1. Gestionar los "Checks" (El tic de seleccionado)
		itemCoffee.Checked = (activeItem == itemCoffee)
		itemBusy.Checked = (activeItem == itemBusy)
		itemMeeting.Checked = (activeItem == itemMeeting)

		// 2. Refrescar el menú para que se vean los cambios (incluyendo Quit)
		desk.SetSystemTrayMenu(fyne.NewMenu("Stagtus",
			itemCoffee,
			itemBusy,
			itemMeeting,
			fyne.NewMenuItemSeparator(),
			itemQuit))

		// 3. Cambiar el icono principal de la bandeja
		desk.SetSystemTrayIcon(activeIcon)

		// 4. Enviar al hardware
		go sendCommand(cmd)
	}

	// Asignamos las acciones ahora
	itemCoffee.Action = func() { updateState(itemCoffee, iconCoffee, "1232") }
	itemBusy.Action = func() { updateState(itemBusy, iconBusy, "1231") }
	itemMeeting.Action = func() { updateState(itemMeeting, iconMeeting, "1233") }

	// Estado Inicial
	itemCoffee.Checked = true

	// 5. CONSTRUIR EL MENÚ FINAL
	menu := fyne.NewMenu("Stagtus",
		itemCoffee,
		itemBusy,
		itemMeeting,
		fyne.NewMenuItemSeparator(),
		itemQuit,
	)

	desk.SetSystemTrayMenu(menu)
	desk.SetSystemTrayIcon(iconCoffee)

	// Conectar USB
	go connectSerial()

	// Arrancar
	a.Run()
}

// --- FUNCIONES DE SOPORTE ---

func loadResource(path string, name string) fyne.Resource {
	data, err := content.ReadFile(path)
	if err != nil {
		log.Println("Error cargando imagen:", err)
		return nil
	}
	return fyne.NewStaticResource(name, data)
}

func connectSerial() {
	mode := &serial.Mode{BaudRate: baudRate}
	var err error
	serialPort, err = serial.Open(portName, mode)
	if err != nil {
		fmt.Println("⚠️ No se pudo conectar al puerto (Modo Simulación):", err)
	} else {
		fmt.Println("✅ Conectado al USB")
	}
}

func sendCommand(cmd string) {
	if serialPort == nil {
		connectSerial() // Reintento rápido
		if serialPort == nil {
			return
		}
	}
	// Importante: \n al final
	_, err := serialPort.Write([]byte(cmd + "\n"))
	if err != nil {
		fmt.Println("Error enviando:", err)
		serialPort.Close()
		serialPort = nil
	} else {
		fmt.Println("Enviado:", cmd)
	}
}
