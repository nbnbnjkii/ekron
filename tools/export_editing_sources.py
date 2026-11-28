#!/usr/bin/env python3
"""Export a labelled, non-build snapshot of Android controls and lobby sources."""
from pathlib import Path
import hashlib
import json
import os
import shutil

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "android-editing"
JAVA = "sys/android/app/src/main/java/com/example/realms/"
CPP = "sys/android/app/src/main/cpp/"
GROUPS = {
    "Управление/Общее": [
        (JAVA + "MainActivity.kt", "Сборка HUD, расположение кнопок, бег-переключатель, джетпак, переключение игрок/машина/FPV, вызовы JNI."),
        (JAVA + "TouchControls.kt", "Отрисовка и касания кнопок HudButton, джойстика StickView и области обзора LookView."),
        (JAVA + "PointerDrag.kt", "Отслеживание пальца при перетаскивании и мультитач."),
        (JAVA + "ControlEditorView.kt", "Редактор положения/размера управления и сохранение профилей player/car/drone."),
        (CPP + "Renderer.cpp", "Общий нативный цикл: JNI-ввод, физика, управление машиной, камеры игрока/FPV/взрыва и рендер."),
        (CPP + "Renderer.h", "Общее состояние рендерера, контроллеров, камер и игровых систем."),
    ],
    "Управление/Игрок": [
        (CPP + "PlayerPhysics.h", "Ходьба, бег, прыжок, приседание, гравитация и коллизии капсулы; движение с джетпаком."),
        (CPP + "PlayerView.h", "Модель игрока, анимация тела и переключение первого/третьего лица."),
        (CPP + "Jetpack.h", "Состояния джетпака, модель и анимированные воздушные струи."),
        (JAVA + "ToolArtView.kt", "Вид инструмента/рук от первого лица и анимация движения/бега."),
        (JAVA + "ToolState.kt", "Состояние выбранного инструмента и видимость действий HUD."),
        (JAVA + "ToolWheelView.kt", "Рисунок и касания колеса выбора инструментов."),
        (JAVA + "ToolWheelSelection.kt", "Логика выбора в колесе инструментов."),
    ],
    "Управление/Дрон": [
        (JAVA + "FpvControlView.kt", "Экранные стики FPV Mode 2 и передача команд управления."),
        (CPP + "FpvDrone.h", "Полёт и состояния FPV, столкновения, возвращение, подключение и камеры дрона."),
        (CPP + "CrashCamera.h", "Дальний ракурс взрыва FPV с поиском свободного направления обзора."),
        (CPP + "BlastEffects.h", "Вспышка, дым, осколки и воздействие взрыва на предметы."),
    ],
    "Управление/Машина": [
        (CPP + "CarSuspension.h", "Четыре колеса, контакты с дорогой, пружины и амортизаторы."),
        (CPP + "CarTraversal.h", "Преодоление невысоких камней и бордюров с проверкой свободного пути."),
        (CPP + "ObjectDynamics.h", "Динамика предметов и машины, столкновения, стабилизация управляемой машины."),
        (CPP + "CityDepenetration.h", "Устранение проникновений и контактные импульсы на геометрии карты."),
        (CPP + "Interaction.h", "Посадка, положение водителя, выход из машины и другие взаимодействия."),
    ],
    "Лобби": [
        (JAVA + "LobbyView.kt", "Интерфейс мониторов, касания, кнопки движения и вызовы нативного лобби."),
        (JAVA + "LobbyIntro.kt", "Непрерывное видео на мониторе, звук, пауза и жизненный цикл декодера."),
        (CPP + "LobbyScene.h", "Комната и коллизии лобби, отдельный контроллер игрока, попадания по экранам."),
        (CPP + "LobbyRenderer.h", "GLES-шейдеры, геометрия комнаты, текстуры и отрисовка мониторов."),
    ],
}


def export():
    manifest = []
    for group, files in GROUPS.items():
        folder = OUT / group
        folder.mkdir(parents=True, exist_ok=True)
        rows = []
        for source, description in files:
            src = ROOT / source
            dst = folder / src.name
            shutil.copyfile(src, dst)
            manifest.append({"copy": str(dst.relative_to(OUT)), "source": source,
                             "sha256": hashlib.sha256(src.read_bytes()).hexdigest()})
            relative = Path(os.path.relpath(src, folder)).as_posix()
            rows.append(f"| [{src.name}]({src.name}) | {description} | [Рабочий исходник]({relative}) |")
        common = ("\nКнопки и JNI-вызовы для этого раздела также находятся в `Управление/Общее/MainActivity.kt`, "
                  "а интеграция физики и камер — в `Управление/Общее/Renderer.cpp`.\n")
        (folder / "README.md").write_text(
            f"# {group}\n\nЭто копии для изучения, не отдельный собираемый модуль.\n" + common +
            "\n| Файл | За что отвечает | Оригинал |\n|---|---|---|\n" + "\n".join(rows) + "\n", encoding="utf-8")
    (OUT / "sources.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Exported {len(manifest)} source copies into {OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    export()
