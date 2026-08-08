

<img width="1150" height="506" alt="Screenshot 2026-01-13 at 13 11 23" src="https://github.com/user-attachments/assets/a15cd5e8-34a2-47d6-af68-6591e4a638c1" />

# OPENCASIO - Sensor Magnético y Clima en Casio F-91W Para Uso Exterior

> [!IMPORTANT]
> Esta página contiene solo la implementación del software. Para detalles del hardware, archivos BOOM, Gerber y de montaje, consulte: https://oshwlab.com/anardine.ef/opencasio-casio-outdoor-replacement-board

## ¿Qué es esto?
OPENCASIO es una placa de reemplazo para el Casio F-91W, uno de los relojes de pulsera más utilizados del mundo.

La placa integra un módulo RTC sellado y de mejor calidad (donde la deriva horaria es sustancialmente menor), así como un sensor de temperatura, presión y humedad junto con un módulo magnético, para que puedas usarlo y rastrear las direcciones hacia las que te encuentras orientado.

## ¿Para qué sirve?
Está destinado a personas que realizan actividades al aire libre y no necesitan un smartwatch voluminoso con funciones inútiles y una autonomía de batería baja. Con OPENCASIO, puedes contar con funciones importantes en un diseño pequeño, ligero y clásico. La idea es maximizar las características del reloj para que sirva como una herramienta importante cuando estás fuera. Puedes monitorear la presión atmosférica para entender si una tormenta se está acercando, o usar la brújula de navegación para guiarte. También hay un LED mejorado que te ayuda a obtener "algo" de luz durante la oscuridad total. Todas las funciones nativas permanecen intactas, como la alarma, el calendario, los ajustes de hora de 12/24 horas y el cronómetro.

## Arquitectura

OPENCASIO se ejecuta en un microcontrolador STM32WB55REV6. A pesar de que este microcontrolador tiene capacidades BLE/WIFI, en su lugar me enfoqué en garantizar que el reloj se controlara lo más posible con un bajo consumo de corriente para que la batería pudiera durar mucho tiempo.

Dado que no hay mucho espacio, se ha utilizado el oscilador interno en lugar de uno externo (HSI). El propio RTC tiene el suyo para el LSE.

Este microcontrolador también es compatible con LCD, por lo que todos los controladores de LCD están mapeados directamente a las funciones alternativas de LCD en la mayoría de los puertos GPIO del microcontrolador.

El siguiente diagrama proporciona una visión general de todas las características y cómo se relacionan con el propio microcontrolador:

```
                                                                  
                            ┌──────────┐                          
 ┌───────────┐              │  BUZZER  │            ┌───────────┐ 
 │           │   I2C        └────▲─────┘            │           │ 
 │   RTC     ◄──────┐            │            ┌─────►   BUTTONS │ 
 │           │      │    ┌───────┼────────┐   │     │           │ 
 └───────────┘      │    │                │   │     └───────────┘ 
                    └────►                │◄──┘                   
                         │                │                        
 ┌───────────┐           │                │         ┌───────────┐ 
 │           │   I2C     │ STM32WB55REV6  │  ┌───┐  │           │ 
 │   MAG     ◄───────────►                ├─►│FET├──►    LED    │ 
 │           │           │                │  └───┘  │           │ 
 └───────────┘           │                │         └───────────┘ 
                         │                │                        
                    ┌────►                │                        
 ┌───────────┐      │    │                ├────┐    ┌───────────┐ 
 │   T,P,H   │      │    └───────▲┌───────┘    │    │           │ 
 │  SENSOR   ◄──────┘            ││            └────►   LCD     │ 
 │           │   I2C       ┌─────┼▼─────┐           │           │ 
 └───────────┘             │            │           └───────────┘ 
                           │  ST-LINK   │                          
                           │            │                          
                           └────────────┘                          
                                                                  
```

### Firmware

El código se ejecuta usando `platformio` y la interfaz `ST-Link` para cargar y depurar.

Por el momento, el código aún está en desarrollo. Las PR (solicitudes de extracción) son bienvenidas.


### Cómo Contribuir

Este es un proyecto difícil de ensamblar a mano. Todas las piezas se obtuvieron y ensamblaron utilizando la increíble fábrica de JLCPCB. Dado que cuesta alrededor de $50 por placa (completamente lista y ensamblada, alrededor de $250 por cinco), siéntete libre de realizar pedidos para dividir el costo. Eres bienvenido a desarrollarlo usando piezas más grandes o a tomar el reto de soldarlas. Dado que es una placa de una sola cara, también puedes usar una placa caliente.
