#!/bin/sh

# Define the dialog options
DIALOG=${DIALOG=dialog}
TITLE="Configuration Menu"
MENU="Choose one of the following options:"
OPTION_1="1"
OPTION_1_TEXT="Compile with mosquitto client (default)"
OPTION_2="2"
OPTION_2_TEXT="Compile without mosquitto client"
OPTION_3="3"
OPTION_3_TEXT="Set IP and Port"
OPTION_4="4"
OPTION_4_TEXT="About us"

# Define the function to display the menu
show_menu() {
    $DIALOG --clear --title "$TITLE" --menu "$MENU" 15 40 4 \
        "$OPTION_1" "$OPTION_1_TEXT" \
        "$OPTION_2" "$OPTION_2_TEXT" \
        "$OPTION_3" "$OPTION_3_TEXT" \
        "$OPTION_4" "$OPTION_4_TEXT" \
        2>&1 >/dev/tty
}

# Define the main loop
while true; do
    # Display the menu and capture the selected option
    selection=$(show_menu)

    # Handle the selected option
    case $selection in
        $OPTION_1)
            # Do something for option 1
            echo "You selected option 1"
            make CPPFLAGS=-DMOSQUITTO
            break
            ;;
        $OPTION_2)
            # Do something for option 2
            echo "You selected option 2"
            make CPPFLAGS=-DNON_MOSQUITTO
            break
            ;;
        $OPTION_3)
            # Do something for option 3
            echo "You selected option 3"
            break
            ;;
        $OPTION_4)
            # Do something for option 4
            echo "You selected option 4"
            make CPPFLAGS=-DOPTION_4
            break
            ;;
        *)
            # Handle invalid selections
            echo "Invalid selection: $selection"
            ;;
    esac
done
