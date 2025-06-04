# Velopack C++ setup steps

For any help with the instructions below, refer to the online documentation for Velopack: https://docs.velopack.io/

## Initial setup
- Download, import and use the lib in your project
- Build your app
- Pack your first version using `vpk pack`. This creates a `Releases` folder with your first version.
- If you're not hosting your releases anywhere, host your `Releases` folder in a local server (eg. HTTP on port 8080)
- Install the app by running the setup executable inside the `Releases` folder (the app is installed under `AppData/Local/your-app-name`)

## After an update
- Rebuild your app
- Pack the new update under a new version number
- Make sure the server hosting the updates is still running
- Run the app again under `AppData/Local/your-app-name/current`. The app should detect the update and apply it. Running the app again should use the new version.

## Debug
- Make sure the correct DLL file is copied in the app executable folder (`AppData/Local/your-app-name/current` by default) depending on your OS, architecture and compiler. Once copied, it should be renamed `velopack_libc.dll`.
- To avoid copying the DLL everytime, a convenient way is to include it inside your build folder and run `vpk pack` inside that folder.
- If having compilation issues, refer to this repository `CMakeLists`.
