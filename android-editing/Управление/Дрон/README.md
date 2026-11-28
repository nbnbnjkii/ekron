# Управление/Дрон

Это копии для изучения, не отдельный собираемый модуль.

Кнопки и JNI-вызовы для этого раздела также находятся в `Управление/Общее/MainActivity.kt`, а интеграция физики и камер — в `Управление/Общее/Renderer.cpp`.

| Файл | За что отвечает | Оригинал |
|---|---|---|
| [FpvControlView.kt](FpvControlView.kt) | Экранные стики FPV Mode 2 и передача команд управления. | [Рабочий исходник](../../../sys/android/app/src/main/java/com/example/realms/FpvControlView.kt) |
| [FpvDrone.h](FpvDrone.h) | Полёт и состояния FPV, столкновения, возвращение, подключение и камеры дрона. | [Рабочий исходник](../../../sys/android/app/src/main/cpp/FpvDrone.h) |
| [CrashCamera.h](CrashCamera.h) | Дальний ракурс взрыва FPV с поиском свободного направления обзора. | [Рабочий исходник](../../../sys/android/app/src/main/cpp/CrashCamera.h) |
| [BlastEffects.h](BlastEffects.h) | Вспышка, дым, осколки и воздействие взрыва на предметы. | [Рабочий исходник](../../../sys/android/app/src/main/cpp/BlastEffects.h) |
