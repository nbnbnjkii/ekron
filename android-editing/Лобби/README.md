# Лобби

Это копии для изучения, не отдельный собираемый модуль.

Кнопки и JNI-вызовы для этого раздела также находятся в `Управление/Общее/MainActivity.kt`, а интеграция физики и камер — в `Управление/Общее/Renderer.cpp`.

| Файл | За что отвечает | Оригинал |
|---|---|---|
| [LobbyMonitorArt.kt](LobbyMonitorArt.kt) | Новое оформление мониторов лобби; экран настроек намеренно исключён. | [Рабочий исходник](../../sys/android/app/src/main/java/com/example/realms/LobbyMonitorArt.kt) |
| [LobbyArtwork.h](LobbyArtwork.h) | Фоновое декодирование атласа рисунков и надписей из макета. | [Рабочий исходник](../../sys/android/app/src/main/cpp/LobbyArtwork.h) |
| [LobbyDecor.h](LobbyDecor.h) | Декорации бункера и анимация центральной голограммы. | [Рабочий исходник](../../sys/android/app/src/main/cpp/LobbyDecor.h) |
| [LobbyView.kt](LobbyView.kt) | Интерфейс мониторов, касания, кнопки движения и вызовы нативного лобби. | [Рабочий исходник](../../sys/android/app/src/main/java/com/example/realms/LobbyView.kt) |
| [LobbyIntro.kt](LobbyIntro.kt) | Непрерывное видео на мониторе, звук, пауза и жизненный цикл декодера. | [Рабочий исходник](../../sys/android/app/src/main/java/com/example/realms/LobbyIntro.kt) |
| [LobbyScene.h](LobbyScene.h) | Комната и коллизии лобби, отдельный контроллер игрока, попадания по экранам. | [Рабочий исходник](../../sys/android/app/src/main/cpp/LobbyScene.h) |
| [LobbyRenderer.h](LobbyRenderer.h) | GLES-шейдеры, геометрия комнаты, текстуры и отрисовка мониторов. | [Рабочий исходник](../../sys/android/app/src/main/cpp/LobbyRenderer.h) |
