package com.example.realms

data class CatalogItem(val nativeType: Int, val label: String, val asset: String, val defaultSurface: Int = 1)

object BuildCatalog {
    // -1 means the model owns its materials, never an array index.
    fun <T> materialPreview(images: Array<T>, surface: Int): T? = images.getOrNull(surface)
    // Vehicle stays available under construction; no invented weapons/drones.
    fun indicesForCategory(category: Int): List<Int> = entries.indices.filter {
        when(category) {
            0 -> entries[it].nativeType !in setOf(4,5,6,7,13,18,22,25,28)
            1 -> entries[it].nativeType in setOf(4,5,6,7,13,18,22,25,28)
            else -> false
        }
    }
    // IDs match World::bounds and the spawn-gun selection protocol.
    val entries = listOf(
        CatalogItem(0, "Бетонный блок", "catalog/rendered/wall.webp"),
        CatalogItem(1, "Плита (пол)", "catalog/rendered/floor.webp"),
        CatalogItem(2, "Окно", "catalog/rendered/window.webp"),
        CatalogItem(3, "Дверь", "catalog/rendered/door.webp",5),
        CatalogItem(4, "Стол", "catalog/rendered/table.webp",5),
        CatalogItem(5, "Стул", "catalog/rendered/chair.webp",5),
        CatalogItem(6, "Шкаф", "catalog/rendered/cabinet.webp",5),
        CatalogItem(7, "Диван", "catalog/rendered/sofa.webp",6),
        CatalogItem(9, "Деревянный блок", "catalog/rendered/wood_block.webp",5),
        CatalogItem(10, "Кирпичный блок", "catalog/rendered/brick_block.webp",2),
        CatalogItem(11, "Металлический блок", "catalog/rendered/metal_block.webp",3),
        CatalogItem(8, "УАЗ-469", "catalog/rendered/uaz.webp",-1),
        CatalogItem(12, "Куб 1×1×1", "catalog/rendered/cube.webp",1),
        CatalogItem(13, "Деревянный ящик", "catalog/rendered/crate.webp",5),
        CatalogItem(14, "Металлическая бочка", "catalog/rendered/barrel.webp",3),
        CatalogItem(15, "Деревянный поддон", "catalog/rendered/pallet.webp",5),
        CatalogItem(16, "Связка стальных труб", "catalog/rendered/pipes.webp",3),
        CatalogItem(17, "Строительный брус", "catalog/rendered/plank.webp",5),
        CatalogItem(18, "Мешок цемента", "catalog/rendered/cement_bag.webp",1),
        CatalogItem(19, "Строительные леса", "catalog/rendered/scaffold.webp",3),
        CatalogItem(20, "Дорожный конус", "catalog/rendered/cone.webp",3),
        CatalogItem(21, "Уличная урна", "catalog/rendered/bin.webp",3),
        CatalogItem(22, "Уличная скамейка", "catalog/rendered/bench.webp",5),
        CatalogItem(23, "Люк", "catalog/rendered/manhole.webp",3),
        CatalogItem(24, "Бетонный барьер", "catalog/rendered/barrier.webp",1),
        CatalogItem(25, "Шина", "catalog/rendered/tire.webp",3),
        CatalogItem(26, "Столб ограждения", "catalog/rendered/post.webp",3),
        CatalogItem(27, "Стальная балка", "catalog/rendered/beam.webp",3),
        CatalogItem(28, "Мусорный контейнер", "catalog/rendered/dumpster.webp",3),
        CatalogItem(29, "Взрывная бочка", "catalog/rendered/explosive_barrel.webp",-1),
        CatalogItem(30, "Малый деревянный ящик", "catalog/rendered/crate_wood_small.webp",5),
        CatalogItem(31, "Большой деревянный ящик", "catalog/rendered/crate_wood_large.webp",5),
        CatalogItem(32, "Малый металлический ящик", "catalog/rendered/crate_metal_small.webp",3),
        CatalogItem(33, "Большой металлический ящик", "catalog/rendered/crate_metal_large.webp",3),
        CatalogItem(34, "Бетонный блок малый", "catalog/rendered/block_small.webp",1),
        CatalogItem(35, "Бетонный блок большой", "catalog/rendered/block_large.webp",1),
        CatalogItem(36, "Мешки с песком", "catalog/rendered/sandbags.webp",-1),
        CatalogItem(37, "Колючая проволока", "catalog/rendered/razor_wire.webp",-1),
        CatalogItem(38, "Противотанковый ёж", "catalog/rendered/hedgehog.webp",-1),
        CatalogItem(39, "Ящик с патронами", "catalog/rendered/ammo_crate.webp",-1),
        CatalogItem(40, "Ящик снарядов", "catalog/rendered/shell_crate.webp",-1),
        CatalogItem(41, "Бетонный надолб", "catalog/rendered/dragon_tooth.webp",-1),
        CatalogItem(42, "Армейская канистра", "catalog/rendered/jerrycan.webp",-1),
        CatalogItem(43, "Армейская палатка", "catalog/rendered/tent.webp",-1),
        CatalogItem(44, "Маскировочная сеть", "catalog/rendered/camo_net.webp",-1),
        CatalogItem(45, "Ящики снабжения", "catalog/rendered/supply_stack.webp",-1),
        CatalogItem(46, "Бочка с топливом", "catalog/rendered/fuel_drum.webp",-1),
        CatalogItem(47, "Медицинский ящик", "catalog/rendered/medkit.webp",-1)
    )
}
