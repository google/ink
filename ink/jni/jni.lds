VERS_1.0 {
  # Export JNI symbols and ANativeActivity_onCreate.
  # Android's NativeActivity expects ANativeActivity_onCreate as its entrypoint.
  global:
    Java_*;
    JNI_OnLoad;
    JNI_OnUnload;
    ANativeActivity_onCreate;

  # Hide everything else.
  local:
    *;
};
