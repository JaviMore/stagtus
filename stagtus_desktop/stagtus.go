package main

import (
	"embed"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"reflect"
	"sort"
	"strings"

	"fyne.io/fyne/v2"
	"fyne.io/fyne/v2/app"
	"fyne.io/fyne/v2/container"
	"fyne.io/fyne/v2/driver/desktop"
	"fyne.io/fyne/v2/layout"
	"fyne.io/fyne/v2/widget"
	"github.com/BurntSushi/toml"
	"go.bug.st/serial"
)

// 1. EMBEDDING
// Embed images into the executable automatically
//
//go:embed images/*.png
var content embed.FS

// Serial port configuration
var selectedPort = "COM13" // Default
const baudRate = 115200

var serialPort serial.Port

// Command represents an entry in the TOML
type Command struct {
	// Emoji ahora contiene el nombre del archivo de imagen dentro de images/, p.ej. "coffee.png"
	Emoji string `toml:"emoji"`
	ID    string `toml:"id"`
	GIF   string `toml:"gif"`
	Text  string `toml:"text"`
	Color string `toml:"color"`
}

// Config represents the TOML configuration
type Config struct {
	General  map[string]interface{}
	Commands map[string]Command
}

var currentState *string = nil       // Pointer to the currently active command
var commandConfig map[string]Command // Store command configuration

func main() {
	// Start the app (required even if we don't show a window)
	myApp := app.New()

	// Prevent the app from closing when windows are closed (even if none exist)
	myApp.SetIcon(loadResource("images/stagtus.png", "icon"))

	// Verify desktop support
	var desk desktop.App
	if d, ok := myApp.(desktop.App); ok {
		desk = d
	} else {
		log.Fatal("Your system does not support System Tray")
	}

	// Create serial port configuration window
	configWindow := myApp.NewWindow("Stagtus - Serial Port Configuration")
	configWindow.Resize(fyne.NewSize(400, 200))

	// Get available ports
	availablePorts := getAvailablePorts()

	// Create Select widget
	var selectedPortName string
	if len(availablePorts) > 0 {
		selectedPortName = availablePorts[0]
	}

	// Prepare port select widget
	portSelectWidget := createPortSelectWidget(availablePorts, &selectedPortName, &selectedPort)

	// Button to refresh the list of ports (emoji icon added)
	refreshBtn := widget.NewButton("♻️ Refresh", func() {
		ports := getAvailablePorts()
		// the widget was created as *widget.Select but stored as fyne.CanvasObject,
		// so cast back before manipulating fields.
		if sel, ok := portSelectWidget.(*widget.Select); ok {
			sel.Options = ports
			sel.Refresh()
			if len(ports) > 0 {
				sel.SetSelected(ports[0])
				selectedPortName = ports[0]
				selectedPort = ports[0]
			}
		}
	})

	// Confirmation button
	confirmBtn := createConfirmButton(myApp, configWindow, desk, &selectedPortName, &selectedPort)

	// put both buttons side-by-side and make each expand to fill width
	buttonRow := container.New(layout.NewGridLayout(2), refreshBtn, confirmBtn)

	// Window layout
	var content fyne.CanvasObject
	if len(availablePorts) > 0 {
		content = container.NewVBox(
			widget.NewLabel("Available COM Ports:"),
			portSelectWidget,
			layout.NewSpacer(),    // push buttons to bottom
			widget.NewSeparator(), // visual gap before buttons
			buttonRow,
		)
	} else {
		content = container.NewVBox(
			widget.NewLabel("No COM ports found!"),
			widget.NewLabel("Using default: COM13"),
			layout.NewSpacer(),
			widget.NewSeparator(),
			buttonRow,
		)
		selectedPort = "COM13"
	}

	configWindow.SetContent(content)
	configWindow.Show()

	// Run the app
	myApp.Run()
}

// --- SUPPORT FUNCTIONS ---

func loadResource(path string, name string) fyne.Resource {
	// embed.FS always uses forward slashes. Normalize Windows paths.
	fsPath := strings.ReplaceAll(path, "\\", "/")
	data, err := content.ReadFile(fsPath)
	if err != nil {
		log.Println("Error loading image:", err)
		return nil
	}
	return fyne.NewStaticResource(name, data)
}

func connectSerial() {
	mode := &serial.Mode{BaudRate: baudRate}
	var err error
	serialPort, err = serial.Open(selectedPort, mode)
	if err != nil {
		fmt.Println("⚠️ Could not open serial port (simulation mode):", err)
	} else {
		fmt.Println("✅ Connected to USB on port:", selectedPort)
	}
}

func sendCommand(cmd string) {
	if serialPort == nil {
		connectSerial() // quick retry
		if serialPort == nil {
			return
		}
	}
	// Important: newline at the end
	_, err := serialPort.Write([]byte(cmd + "\n"))
	if err != nil {
		fmt.Println("Error sending:", err)
		serialPort.Close()
		serialPort = nil
	} else {
		fmt.Println("Sent:", cmd)
	}
}

// --- PORT SELECTION HELPERS ---

// getAvailablePorts returns a list of available COM ports
func getAvailablePorts() []string {
	ports, err := serial.GetPortsList()
	if err != nil {
		log.Println("Error getting ports list:", err)
		return []string{}
	}
	return ports
}

// createPortSelectWidget creates a widget to select the serial port
func createPortSelectWidget(ports []string, selectedPort *string, globalPort *string) fyne.CanvasObject {
	select_widget := widget.NewSelect(ports, func(s string) {
		*selectedPort = s
		*globalPort = s
	})
	if len(ports) > 0 {
		select_widget.SetSelected(ports[0])
		*selectedPort = ports[0]
		*globalPort = ports[0]
	}
	return select_widget
}

// createConfirmButton creates the confirmation button
func createConfirmButton(a fyne.App, w fyne.Window, desk desktop.App, selectedPort *string, globalPort *string) fyne.CanvasObject {
	return widget.NewButton("🔌 Connect", func() {
		*globalPort = *selectedPort

		// Load configuration from TOML
		commands, err := loadConfig()
		if err != nil {
			log.Println("Error loading config:", err)
			// If there is no config, create a default configuration
			commands = map[string]Command{
				"coffee":  {Emoji: "coffee.png", ID: "1232", Text: "Coffee"},
				"busy":    {Emoji: "busy.png", ID: "1231", Text: "Busy"},
				"meeting": {Emoji: "meeting.png", ID: "1233", Text: "Meeting"},
			}
		}
		commandConfig = commands // store globally

		// Create menu items dynamically
		menuItems := createMenuItemsFromConfig(commands, desk, a)

		// Mark the first one as active initially
		if len(menuItems) > 0 {
			firstCmd := ""
			for k := range commands {
				if k != "general" {
					firstCmd = k
					break
				}
			}
			menuItems[0].Checked = true
			currentState = &firstCmd
		}

		// Add separator and Quit
		itemQuit := fyne.NewMenuItem("Quit", func() {
			a.Quit()
		})

		allItems := append(menuItems, fyne.NewMenuItemSeparator(), itemQuit)
		menu := fyne.NewMenu("Stagtus", allItems...)

		// Try to use the first image defined in the TOML as the default tray icon
		var defaultIcon fyne.Resource
		// Use stagtus.png as the default tray icon
		defaultIcon = loadResource("images/stagtus.png", "default")
		// Fallback
		if defaultIcon == nil {
			defaultIcon = loadResource("images/coffee.png", "default")
		}

		desk.SetSystemTrayMenu(menu)
		if defaultIcon != nil {
			desk.SetSystemTrayIcon(defaultIcon)
		}

		// Connect to the selected port
		go connectSerial()

		// Hide the configuration window
		w.Hide()
	})
}

// getConfigPath returns the path to config.toml
func getConfigPath() string {
	// Try to find it in the executable directory first
	exePath, err := os.Executable()
	if err == nil {
		configPath := filepath.Join(filepath.Dir(exePath), "config.toml")
		if _, err := os.Stat(configPath); err == nil {
			return configPath
		}
	}

	// If not found there, check the current working directory
	if _, err := os.Stat("config.toml"); err == nil {
		return "config.toml"
	}

	// Default to config.toml in the working directory
	return "config.toml"
}

// loadConfig loads configuration from the TOML file
func loadConfig() (map[string]Command, error) {
	configPath := getConfigPath()
	fileContent, err := os.ReadFile(configPath)
	if err != nil {
		return nil, fmt.Errorf("error reading config.toml: %v", err)
	}

	var config map[string]Command
	err = toml.Unmarshal(fileContent, &config)
	if err != nil {
		return nil, fmt.Errorf("error parsing config.toml: %v", err)
	}

	return config, nil
}

// createMenuItemsFromConfig creates menu items based on TOML configuration
func createMenuItemsFromConfig(commands map[string]Command, desk desktop.App, a fyne.App) []*fyne.MenuItem {
	var menuItems []*fyne.MenuItem

	// Sort command names to ensure consistent order
	var sortedKeys []string
	for cmdName := range commands {
		if cmdName != "general" {
			sortedKeys = append(sortedKeys, cmdName)
		}
	}
	sort.Strings(sortedKeys)

	// Create menu items for each command (except "general") in sorted order
	for _, cmdName := range sortedKeys {
		cmdConfig := commands[cmdName]

		// Capture variables for closure
		name := cmdName
		config := cmdConfig

		// Label shows only the text; the image is specified in the Emoji field (filename)
		label := config.Text

		// Create the menu item
		item := fyne.NewMenuItem(label, func() {
			// Update current state
			currentState = &name

			// If an image is associated, update the tray icon
			if config.Emoji != "" {
				icon := loadResource(filepath.Join("images", config.Emoji), config.Emoji)
				if icon != nil {
					desk.SetSystemTrayIcon(icon)
				}
			}

			// Rebuild the menu with updated checks
			updateMenuChecks(commands, desk, a, name)

			// Send the command
			go sendCommand(config.ID)
		})

		// If an image filename is provided, try to set it as the menu item's icon
		if config.Emoji != "" {
			if icon := loadResource(filepath.Join("images", config.Emoji), config.Emoji); icon != nil {
				// Assign to Icon field if present (some fyne versions support it)
				// This will be a no-op at runtime if the field is not used by the platform.
				// Use reflection assignment to avoid compile errors on older fyne versions.
				// Try direct assignment first (preferred for newer fyne):
				// item.Icon = icon
				// If direct assignment isn't available, fall back to reflection below.
				setMenuItemIcon(item, icon)
			}
		}

		menuItems = append(menuItems, item)
	}

	return menuItems
}

// updateMenuChecks updates the check marks in the menu
func updateMenuChecks(commands map[string]Command, desk desktop.App, a fyne.App, activeCommand string) {
	menuItems := createMenuItemsFromConfig(commands, desk, a)

	// Mark active
	for i, item := range menuItems {
		cmdName := getCommandNameByIndex(commands, i)
		item.Checked = (cmdName == activeCommand)
	}

	// Add separator and Quit
	itemQuit := fyne.NewMenuItem("Quit", func() {
		a.Quit()
	})

	// Build final menu
	allItems := append(menuItems, fyne.NewMenuItemSeparator(), itemQuit)
	menu := fyne.NewMenu("Stagtus", allItems...)

	desk.SetSystemTrayMenu(menu)

	// Update tray icon according to the active command (if an image exists)
	if activeCommand != "" {
		if cfg, ok := commands[activeCommand]; ok {
			if cfg.Emoji != "" {
				icon := loadResource(filepath.Join("images", cfg.Emoji), cfg.Emoji)
				if icon != nil {
					desk.SetSystemTrayIcon(icon)
				}
			}
		}
	}
}

// getCommandNameByIndex returns the command name at the given index
func getCommandNameByIndex(commands map[string]Command, index int) string {
	i := 0
	for k := range commands {
		if k != "general" {
			if i == index {
				return k
			}
			i++
		}
	}
	return ""
}

// setMenuItemIcon attempts to set the icon on a menu item using reflection
// to remain compatible with different fyne versions.
func setMenuItemIcon(item *fyne.MenuItem, icon fyne.Resource) {
	defer func() { _ = recover() }()

	// fast path: if the item implements a SetIcon method, use it
	type mayHaveSetIcon interface{ SetIcon(fyne.Resource) }
	if v, ok := interface{}(item).(mayHaveSetIcon); ok {
		v.SetIcon(icon)
		return
	}

	// fallback: set exported Icon field via reflection if available
	rv := reflect.ValueOf(item)
	if rv.Kind() == reflect.Ptr {
		rv = rv.Elem()
	}
	if rv.IsValid() {
		f := rv.FieldByName("Icon")
		if f.IsValid() && f.CanSet() {
			f.Set(reflect.ValueOf(icon))
			return
		}
	}
}
