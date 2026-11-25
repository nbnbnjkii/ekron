package com.example.realms
import org.junit.Assert.*
import org.junit.Test
class BuildCatalogTest {
    @Test fun spawnPreviewsAllowModelOwnedMaterials() {
        val images=arrayOfNulls<String>(7)
        images[1]="concrete";images[3]="metal";images[5]="wood";images[6]="fabric"
        for(category in 0..3)for(index in BuildCatalog.indicesForCategory(category)){
            val item=BuildCatalog.entries[index]
            val preview=BuildCatalog.materialPreview(images,item.defaultSurface)
            if(item.defaultSurface==-1)assertNull(preview)
            else assertEquals(images[item.defaultSurface],preview)
        }
        val barrel=BuildCatalog.entries.single { it.nativeType==29 }
        assertEquals(-1,barrel.defaultSurface)
        assertNull(BuildCatalog.materialPreview(images,barrel.defaultSurface))
        assertNull(BuildCatalog.materialPreview(images,Int.MIN_VALUE))
        assertNull(BuildCatalog.materialPreview(images,images.size))
    }
    @Test fun picturesMatchNativeTypesWithoutDuplicateAssets() {
        val items=BuildCatalog.entries
        assertEquals(listOf(0,1,2,3,4,5,6,7,9,10,11,8,12)+(13..47).toList(),items.map { it.nativeType })
        assertEquals(48,items.map { it.asset }.toSet().size)
        assertEquals(listOf("Бетонный блок","Плита (пол)","Окно","Дверь","Стол","Стул","Шкаф","Диван","Деревянный блок","Кирпичный блок","Металлический блок","УАЗ-469","Куб 1×1×1","Деревянный ящик","Металлическая бочка","Деревянный поддон","Связка стальных труб","Строительный брус","Мешок цемента","Строительные леса","Дорожный конус","Уличная урна","Уличная скамейка","Люк","Бетонный барьер","Шина","Столб ограждения","Стальная балка","Мусорный контейнер","Взрывная бочка","Малый деревянный ящик","Большой деревянный ящик","Малый металлический ящик","Большой металлический ящик","Бетонный блок малый","Бетонный блок большой","Мешки с песком","Колючая проволока","Противотанковый ёж","Ящик с патронами","Ящик снарядов","Бетонный надолб","Армейская канистра","Армейская палатка","Маскировочная сеть","Ящики снабжения","Бочка с топливом","Медицинский ящик"),items.map { it.label })
        assertEquals(listOf(1,1,1,5,5,5,5,6,5,2,3,-1,1,5,3,5,3,5,1,3,3,3,5,3,1,3,3,3,3,-1,5,5,3,3,1,1)+List(12){-1},items.map { it.defaultSurface })
        assertTrue(items.all { it.asset.startsWith("catalog/") || it.asset.startsWith("textures/") })
    }
}
