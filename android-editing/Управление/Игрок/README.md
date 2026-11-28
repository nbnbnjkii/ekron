# Управление/Игрок

Это копии для изучения, не отдельный собираемый модуль.

Кнопки и JNI-вызовы для этого раздела также находятся в `Управление/Общее/MainActivity.kt`, а интеграция физики и камер — в `Управление/Общее/Renderer.cpp`.

| Файл | За что отвечает | Оригинал |
|---|---|---|
| [PlayerPhysics.h](PlayerPhysics.h) | Ходьба, бег, прыжок, приседание, гравитация и коллизии капсулы; движение с джетпаком. | [Рабочий исходник](../../../sys/android/app/src/main/cpp/PlayerPhysics.h) |
| [PlayerView.h](PlayerView.h) | Модель игрока, анимация тела и переключение первого/третьего лица. | [Рабочий исходник](../../../sys/android/app/src/main/cpp/PlayerView.h) |
| [Jetpack.h](Jetpack.h) | Состояния джетпака, модель и анимированные воздушные струи. | [Рабочий исходник](../../../sys/android/app/src/main/cpp/Jetpack.h) |
| [ToolArtView.kt](ToolArtView.kt) | Вид инструмента/рук от первого лица и анимация движения/бега. | [Рабочий исходник](../../../sys/android/app/src/main/java/com/example/realms/ToolArtView.kt) |
| [ToolState.kt](ToolState.kt) | Состояние выбранного инструмента и видимость действий HUD. | [Рабочий исходник](../../../sys/android/app/src/main/java/com/example/realms/ToolState.kt) |
| [ToolWheelView.kt](ToolWheelView.kt) | Рисунок и касания колеса выбора инструментов. | [Рабочий исходник](../../../sys/android/app/src/main/java/com/example/realms/ToolWheelView.kt) |
| [ToolWheelSelection.kt](ToolWheelSelection.kt) | Логика выбора в колесе инструментов. | [Рабочий исходник](../../../sys/android/app/src/main/java/com/example/realms/ToolWheelSelection.kt) |
