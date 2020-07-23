Dentist
===================

An open source kernel extension providing user level access to IOBluetoothFamily functions.

It's called Dentist, because it pokes around in Bluetooth, just like a dentist does.

#### Features
- Planned to attach to Frankenstein for Bluetooth firmware fuzzing.
- It is also possible to fuzz IOBluetoothFamily drivers of course.
- Use `corpus_creator` script to save data from chip to driver to a file.
- Use this command to see debug output from `Dentist`:

    `log stream --info --debug --predicate 'senderImagePath contains "Dentist"'`

#### Boot-args
- `-dentistoff` disables kext loading
- `-dentistdbg` turns on debugging output
- `-dentistbeta` enables loading on unsupported osx

#### Credits
- [Apple](https://www.apple.com) for macOS  
- [vit9696](https://github.com/vit9696) for [Lilu.kext](https://github.com/vit9696/Lilu)
- [lvs1974](https://applelife.ru/members/lvs1974.53809/) for helpful advice and tips
