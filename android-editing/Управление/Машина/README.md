# Управление/Машина

Это копии для изучения, не отдельный собираемый модуль.

Кнопки и JNI-вызовы для этого раздела также находятся в `Управление/Общее/MainActivity.kt`, а интеграция физики и камер — в `Управление/Общее/Renderer.cpp`.

| Файл | За что отвечает | Оригинал |
|---|---|---|
| [CarSuspension.h](CarSuspension.h) | Четыре колеса, контакты с дорогой, пружины и амортизаторы. | [Рабочий исходник](../../../sys/android/app/src/main/cpp/CarSuspension.h) |
| [CarTraversal.h](CarTraversal.h) | Преодоление невысоких камней и бордюров с проверкой свободного пути. | [Рабочий исходник](../../../sys/android/app/src/main/cpp/CarTraversal.h) |
| [ObjectDynamics.h](ObjectDynamics.h) | Динамика предметов и машины, столкновения, стабилизация управляемой машины. | [Рабочий исходник](../../../sys/android/app/src/main/cpp/ObjectDynamics.h) |
| [CityDepenetration.h](CityDepenetration.h) | Устранение проникновений и контактные импульсы на геометрии карты. | [Рабочий исходник](../../../sys/android/app/src/main/cpp/CityDepenetration.h) |
| [Interaction.h](Interaction.h) | Посадка, положение водителя, выход из машины и другие взаимодействия. | [Рабочий исходник](../../../sys/android/app/src/main/cpp/Interaction.h) |
