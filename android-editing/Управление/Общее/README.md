# Управление/Общее

Это копии для изучения, не отдельный собираемый модуль.

Кнопки и JNI-вызовы для этого раздела также находятся в `Управление/Общее/MainActivity.kt`, а интеграция физики и камер — в `Управление/Общее/Renderer.cpp`.

| Файл | За что отвечает | Оригинал |
|---|---|---|
| [MainActivity.kt](MainActivity.kt) | Сборка HUD, расположение кнопок, бег-переключатель, джетпак, переключение игрок/машина/FPV, вызовы JNI. | [Рабочий исходник](../../../sys/android/app/src/main/java/com/example/realms/MainActivity.kt) |
| [TouchControls.kt](TouchControls.kt) | Отрисовка и касания кнопок HudButton, джойстика StickView и области обзора LookView. | [Рабочий исходник](../../../sys/android/app/src/main/java/com/example/realms/TouchControls.kt) |
| [PointerDrag.kt](PointerDrag.kt) | Отслеживание пальца при перетаскивании и мультитач. | [Рабочий исходник](../../../sys/android/app/src/main/java/com/example/realms/PointerDrag.kt) |
| [ControlEditorView.kt](ControlEditorView.kt) | Редактор положения/размера управления и сохранение профилей player/car/drone. | [Рабочий исходник](../../../sys/android/app/src/main/java/com/example/realms/ControlEditorView.kt) |
| [Renderer.cpp](Renderer.cpp) | Общий нативный цикл: JNI-ввод, физика, управление машиной, камеры игрока/FPV/взрыва и рендер. | [Рабочий исходник](../../../sys/android/app/src/main/cpp/Renderer.cpp) |
| [Renderer.h](Renderer.h) | Общее состояние рендерера, контроллеров, камер и игровых систем. | [Рабочий исходник](../../../sys/android/app/src/main/cpp/Renderer.h) |
