# Conversación: tutoría socrática sobre MLFQ en XV6 (RISC-V)

Exportación completa de la conversación (turnos 1 a 16). Las imágenes adjuntas se indican con una nota y se transcribe su contenido; el archivo del enunciado del taller se menciona sin reproducirlo.

---

## Estudiantes (mensaje inicial)

Actúa como tutor socrático de una pareja de estudiantes de cuarto semestre de Ingeniería de Sistemas. Conocemos C++ y hemos comenzado a explorar XV6 para RISC-V.

Queremos identificar y justificar qué se necesitaría para incorporar MLFQ a XV6. En esta entrega no vamos a implementarlo.

Nuestra descripción inicial del algoritmo, escrita antes de esta conversación:

"Entendemos MLFQ (Multilevel Feedback Queue) como un planificador con varias colas, cada una con una prioridad distinta. Las reglas que conocemos son:
1. Si un proceso tiene mayor prioridad que otro, corre primero el de mayor prioridad.
2. Si dos procesos tienen la misma prioridad, se turnan con Round Robin.
3. Un proceso nuevo entra en la cola de mayor prioridad.
4. Si un proceso gasta todo su quantum, baja a la cola siguiente. Si cede la CPU antes de gastarlo (por ejemplo, porque espera E/S), se queda en la misma cola.
5. De vez en cuando, todos los procesos suben a la cola de mayor prioridad para que los de abajo no se queden sin CPU.
La idea es favorecer a los procesos interactivos (que usan poca CPU y se bloquean pronto) sin saber de antemano cuánto va a durar cada proceso.

Dudas que tenemos:
- ¿Cuántas colas debería haber y de cuánto debería ser el quantum en cada una?
- ¿El quantum se cuenta por turno o se acumula en el nivel? Si un proceso cede la CPU justo antes de terminar su quantum muchas veces, ¿podría quedarse siempre arriba?
- ¿Cada cuánto tiempo debería ocurrir el aumento de prioridad (boost) y a quién se aplica?
- Cuando un proceso se despierta de una espera, ¿en qué cola queda y en qué posición dentro de ella?
- Si un proceso de mayor prioridad se vuelve ejecutable mientras otro de menor prioridad está en la CPU, ¿lo expropia de inmediato o espera a que termine su quantum?"

Lo que averiguamos del planificador original y de su incorporación al sistema compilado (commit fa2ddcb7ca87a77af579380a6eafd5aff163b44f del repositorio mit-pdos/xv6-riscv):

- XV6 usa Round Robin sin prioridades. Lo comprobamos ejecutando con una sola CPU virtual (`make qemu CPUS=1`) un programa con tres procesos hijos que solo gastan CPU: sus salidas aparecen intercaladas y cíclicas, y cada proceso corre un tick por turno.
- La selección del siguiente proceso ocurre en `scheduler()` de `kernel/proc.c`: un `for` recorre `proc[NPROC]` en orden de índice y elige el primer proceso con `p->state == RUNNABLE`. Lo pone en `RUNNING`, asigna `c->proc = p` y llama a `swtch(&c->context, &p->context)`. Si no hay ninguno, la CPU ejecuta `wfi`.
- El quantum es el intervalo del temporizador: `timerinit()` (`kernel/start.c`) y `clockintr()` (`kernel/trap.c`) programan la siguiente interrupción con `w_stimecmp(r_time() + 1000000)`, que el código comenta como "about a tenth of a second". En cada interrupción de reloj, `usertrap()` y `kerneltrap()` (`kernel/trap.c`, rama `which_dev == 2`) llaman a `yield()`.
- Estados (`kernel/proc.h`): `UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE`. `yield()` pone `RUNNING → RUNNABLE` y llama a `sched()`; `sleep()` pone `SLEEPING`; `wakeup(chan)` pone `SLEEPING → RUNNABLE`; `kexit()` pone `ZOMBIE`. Todos terminan en `sched()`, que hace `swtch(&p->context, &mycpu()->context)` de vuelta al planificador. El proceso que sale debe haber cambiado su estado antes de llamar a `sched()`, y `p->lock` se mantiene durante el cambio de contexto.
- `clockintr()` incrementa `ticks` solo en la CPU 0 (`cpuid() == 0`) y llama a `wakeup(&ticks)`.
- Cómo llega al sistema final: `kernel/proc.c` y `kernel/swtch.S` están en la lista `OBJS` del `Makefile` (`$K/proc.o`, `$K/swtch.o`); `make` los compila y los enlaza con `$(LD) ... -T $K/kernel.ld -o $K/kernel $(OBJS)`; QEMU carga `kernel/kernel` con `-kernel`; y `main()` en `kernel/main.c` termina llamando a `scheduler()`, que nunca retorna. El planificador es código del kernel enlazado de forma estática.
- La estructura `struct proc` (`kernel/proc.h`) tiene `lock`, `state`, `chan`, `killed`, `xstate` y `pid` entre otros campos, pero no tiene ningún campo de prioridad ni de tiempo de CPU usado (esto lo observamos en el código).

Reglas de interacción:

1. Formula una sola pregunta principal por turno y espera nuestra respuesta. No presentes una lista completa de cambios ni una arquitectura resuelta.
2. Empieza examinando nuestra descripción de MLFQ. Ayúdanos a detectar reglas ambiguas, ausentes o contradictorias. No sustituyas nuestra descripción por una versión terminada ni elijas silenciosamente una variante del algoritmo.
3. Pídenos que propongamos y justifiquemos las decisiones que faltan. Después, ayúdanos a comprobarlas con ejemplos pequeños de procesos y decisiones de planificación que nosotros debamos resolver.
4. Guíanos para relacionar cada regla con XV6. Antes de sugerir una modificación, pregúntanos qué comportamiento necesitamos y qué evidencia del código respalda nuestra propuesta.
5. No supongas que tienes acceso a nuestro repositorio. Pide los fragmentos necesarios y distingue lo observado de lo que todavía sea una hipótesis. No inventes archivos, funciones o comportamientos de nuestra versión.
6. No entregues código de implementación, parches, una lista cerrada de archivos por modificar ni pseudocódigo que resuelva el planificador. Revisa nuestras propuestas y señala problemas mediante preguntas y pistas graduales. Puedes explicar conceptos previos con ejemplos mínimos.
7. Si nos bloqueamos, reduce la dificultad de la pregunta y ofrece una pista concreta. Si pedimos la solución directa, conserva el modo socrático.

El objetivo es que nosotros logremos:

A. Redactar una descripción revisada y coherente de la variante de MLFQ considerada.
B. Explicar qué cambios necesitaría XV6 para soportarla, indicando para cada uno su propósito, su relación con las reglas y la evidencia del código que lo justifica.
C. Proponer escenarios para comprobar una futura implementación del algoritmo MLFQ en el sistema operativo XV6. Pídenos que expliquemos qué procesos participarían, qué harían, qué regla comprobaríamos y qué comportamiento esperaríamos observar. No nos entregues los escenarios resueltos. En esta tarea solo los describiremos; no vamos a programarlos ni ejecutarlos.

Mantén el análisis dentro del alcance de una propuesta: no necesitamos implementar MLFQ ni construir un selector de planificadores. Ayúdanos a registrar las cuestiones que queden pendientes de investigación.

Al finalizar, pídenos que redactemos nuestras conclusiones. Comprueba su consistencia mediante una última pregunta aplicada a un caso concreto, sin escribir por nosotros el reporte.

Comienza con una pregunta que permita comprobar cómo entendemos una regla de nuestra descripción inicial de MLFQ.

---

## Tutor (turno 1)

¡Buen punto de partida! Tienen una descripción clara de la intención del algoritmo, y los datos que recogieron de XV6 son sólidos. Vamos a trabajar sobre sus reglas antes de tocar el código.

Empiezo por las reglas 4 y 5, que usted mismos escribieron:

> 4. Si un proceso gasta todo su quantum, baja a la cola siguiente. Si cede la CPU antes de gastarlo (por ejemplo, porque espera E/S), se queda en la misma cola.

Imaginen un proceso **A** que se comporta así en la cola de mayor prioridad, cuyo quantum es de 10 ticks: corre 9 ticks, hace una E/S breve y cede la CPU; vuelve, corre otros 9 ticks, vuelve a ceder, y así indefinidamente. Entre tanto, otro proceso **B**, que solo calcula y nunca espera E/S, está en la misma cola.

**Pregunta principal:** siguiendo literalmente las reglas 1 a 5 tal como las escribieron, ¿en qué cola estaría A después de, digamos, 100 turnos, y en qué cola estaría B? ¿Les parece que el resultado es coherente con el objetivo que declararon (favorecer a los interactivos)? Justifiquen su respuesta indicando qué regla aplica en cada momento.

Una pista para orientar el razonamiento, sin darles la respuesta: fíjense en qué dice la regla 4 sobre *cuándo* se considera que un proceso "gastó su quantum", y en si la regla distingue entre "cedió porque esperaba E/S" y "cedió por otra razón". Tomen nota también de cualquier cosa que les resulte ambigua mientras razonan; la iremos registrando como cuestión pendiente.

---

## Estudiantes (turno 2)

A: seguiría en la cola de mayor prioridad después de 100 turnos. Entra arriba (regla 3) y en cada turno cede la CPU tras 9 de 10 ticks, sin gastar todo su quantum; la regla 4 solo baja a quien lo gasta completo, así que nunca baja.
B: también entra arriba, pero como solo calcula agota el quantum en cada turno y la regla 4 lo baja hasta la cola de menor prioridad. Por la regla 1, B solo corre cuando A espera E/S, y la regla 5 (boost) lo sube periódicamente.
¿Coherente con el objetivo? No. A usa el 90 % de su quantum, es casi un proceso intensivo en CPU, pero conserva la prioridad máxima. Cualquier proceso podría ceder justo antes de terminar su quantum para quedarse arriba y dejar a los demás casi sin CPU.
Ambigüedad pendiente: la regla 4 cuenta el quantum por turno y no distingue por qué cedió el proceso. Queda por decidir si contar el CPU acumulado en el nivel y qué pasa con el tiempo usado al volver de una espera.

---

## Tutor (turno 2)

Bien razonado. Detectaron la debilidad central de la regla 4 (el juego de ceder justo antes de agotar el quantum) y la formularon con sus propias palabras. Una precisión sobre B: dicen que "solo corre cuando A espera E/S" y eso es correcto por la regla 1. Pero fíjense en que, mientras A espera, B corre en el hueco, y A vuelve a ser prioritario apenas se despierta. Esto nos lleva a la pregunta de la expropiación, que dejamos para más adelante.

Ahora les toca a ustedes decidir. Registro como **pendiente P1**: "la regla 4 cuenta el quantum por turno y no distingue la razón de ceder".

**Pregunta principal:** propongan una forma concreta de contabilizar el uso de CPU de un proceso en un nivel, de modo que A (9 de cada 10 ticks, cediendo siempre antes de agotar) termine bajando de cola. Para ello, respondan:

1. ¿Qué cantidad se acumula, cuándo se reinicia y qué condición dispara el descenso?
2. ¿Por qué esa propuesta no castiga injustamente a un proceso genuinamente interactivo, que usa 1 de cada 10 ticks y se bloquea con frecuencia?

Para comprobar su propuesta, resuelvan este caso mínimo con ella: nivel superior con quantum (asignación total) de 10 ticks; el proceso **C** corre 3 ticks, se bloquea por E/S, vuelve, corre 3 ticks, se bloquea, vuelve, y así sucesivamente. Indiquen en qué momento, si es que ocurre, C baja de cola, y si les parece justo.

Pista por si se bloquean: piensen en la diferencia entre "tiempo que se me da en cada turno" y "tiempo total que he consumido mientras permanezco en este nivel". Y una pregunta que quedará abierta para después: ¿qué pasaría con su propuesta si el proceso C permanece bloqueado durante mucho tiempo antes de volver?

---

## Estudiantes (turno 3)

1. Qué se acumula: los ticks de CPU que el proceso consume en su nivel actual (`ticks_used`), sin importar si cede la CPU antes de tiempo.
Cuándo se reinicia: solo al cambiar de nivel y en el boost; no al bloquearse ni al despertar.
Qué dispara el descenso: cuando `ticks_used` llega al allotment (10 ticks), el proceso baja un nivel.
Caso A: acumula 9 y luego llega a 10 en su segundo turno, así que baja.
2. Interactivo: usa 1 de cada 10 ticks, tardaría unos 100 ticks en llegar a 10 (A tarda unos 11). Si el boost ocurre cada `S` < 100 ticks, vuelve arriba y se reinicia antes de bajar.

Caso C (corre 3 ticks y se bloquea): acumula 3, 6, 9 y en el cuarto turno llega a 10 y baja. Es justo, porque consumió el allotment completo, y un boost corto lo devolvería arriba.
Pendiente: si C pasa mucho tiempo bloqueado, ¿su contador debe reiniciarse o basta con el boost?

---

## Tutor (turno 3)

Su propuesta resuelve P1: acumular CPU consumida por nivel elimina el truco de A, y su cálculo del interactivo es consistente (1 de cada 10 ticks, con S < 100, no llega al allotment). Sin embargo, ese resultado depende de S y no de la regla en sí; lo retomaremos cuando decidan S.

Antes de seguir, una observación sobre su caso C. Dicen que en el cuarto turno "llega a 10 y baja", pero C corre 3 ticks por turno. Ahí aparecen dos cantidades distintas: el **allotment** (10 ticks en el nivel) y el **tiempo por turno** (el que gobierna el Round Robin de la regla 2). En mi enunciado anterior mezclé ambos con la palabra "quantum", así que parte de la confusión es mía. Registro dos pendientes:

- **P2:** ¿cuánto dura el turno, y se relaciona con el allotment? ¿Baja C a mitad de turno o al terminar?
- **P3:** ¿a qué procesos se aplica el boost y qué le pasa a `ticks_used` de cada uno?

**Pregunta principal:** tomen su propia duda (¿C bloqueado mucho tiempo?) y resuélvanla con un caso concreto. C ha acumulado `ticks_used = 9` en el nivel superior y entra en espera de E/S durante 1000 ticks. El boost ocurre cada S = 100 ticks.

1. Durante esos 1000 ticks, ¿qué le ocurre a C en cada boost? ¿Les parece que un proceso `SLEEPING` debe participar en el boost? Justifiquen su decisión.
2. Cuando C despierta, ¿qué valor tiene `ticks_used` y en qué cola queda según su propuesta? ¿Es el resultado que esperarían de un proceso que estuvo dormido tanto tiempo?

Cuando respondan, indiquen qué parte de su respuesta es una **decisión de diseño** y cuál sería una **hipótesis sobre XV6** que todavía no han verificado en el código. Les pediré evidencia para esta última más adelante.

---

## Estudiantes (turno 4)

P2: el quantum es lo que dura un turno en una cola (regla 2) y el allotment es el total de CPU permitido en un nivel. C baja en el tick en que `ticks_used` llega a 10, aunque sea a mitad de turno, y pasa al final de la cola inferior.
1. Boost durante los 1000 ticks: hay 10 boosts. El boost se aplica a todos los procesos no `UNUSED`, incluidos los `SLEEPING`: los sube a la cola más alta y reinicia `ticks_used = 0`. C pasa de 9 a 0 en el primer boost. Si excluyéramos a los dormidos, C bajaría tras un solo tick al despertar aunque estuvo inactivo.
2. Al despertar: `ticks_used = 0` y cola más alta, lo esperable tras tanto tiempo sin usar CPU. Limitación: si duerme menos de `S`, puede despertar con 9. Queda pendiente.
P3: el boost aplica a todos los procesos no `UNUSED` y reinicia nivel y `ticks_used`.
Decisiones de diseño: boost incluye `SLEEPING`, reinicia el contador, el descenso ocurre al alcanzar el allotment, y quantum y allotment son parámetros distintos.
Observado: `wakeup()` recorre todo `proc[]` tomando cada `p->lock`; `clockintr()` ya hace `ticks++` solo en la CPU 0.
Hipótesis sin verificar: que el boost sea seguro dentro de `clockintr()` con los `p->lock`, que el contador por proceso no cuente un tick dos veces entre `usertrap()` y `kerneltrap()`, y que `ticks` sirva para medir `S`.

---

## Tutor (turno 4)

Su decisión sobre el boost está bien justificada: el contraejemplo de C bajando tras un solo tick al despertar es el argumento correcto contra excluir a los dormidos. También separaron bien lo que es decisión de diseño de lo que es hipótesis, y eso se agradece.

**Pendientes registrados**
- **P4:** un proceso que duerme menos de `S` puede despertar con `ticks_used = 9`. Ustedes ya lo señalaron y queda abierto.
- **P5:** "no `UNUSED`" incluye `USED` y `ZOMBIE`. ¿Tiene sentido subir de cola a un proceso que está terminando o que aún se está creando? Esto lo revisaremos al mirar el código.
- **P6:** ustedes afirman que `wakeup()` recorre todo `proc[]` tomando cada `p->lock`. Lo trato como **hipótesis** hasta que me muestren ese fragmento, porque en lo que me describieron solo aparece que pone `SLEEPING → RUNNABLE`.

**Pregunta principal (regla 1 y expropiación)**

Tienen una sola CPU. El proceso **B** está en la cola inferior y lleva 2 ticks de su turno. En ese momento, un evento de E/S despierta a **A**, que estaba `SLEEPING` y pertenece a la cola superior.

1. ¿Debe A quitarle la CPU a B de inmediato o esperar a que B termine su turno? Elijan una opción y justifíquenla con la regla 1 y con el objetivo de favorecer a los interactivos. Indiquen qué costo tiene la opción que descartan.
2. Según su descripción, ¿en qué cola y en qué posición queda A al despertar? ¿Esto depende de si venía de gastar poco o mucho de su turno?

Para relacionarlo con XV6, respondan solo con lo que sepan del código: en el XV6 actual, ¿qué eventos hacen que un proceso `RUNNING` deje la CPU? ¿`wakeup()` es uno de ellos? Si no están seguros, indiquen qué fragmento me mostrarían para comprobarlo.

Pista si se bloquean: reescriban el caso con A de alta prioridad y B de baja y pregúntense cuánto tiempo pasaría A esperando como máximo si B nunca cede la CPU por su cuenta.

---

## Estudiantes (turno 5)

*(Adjunto: imagen con el código de `wakeup()`, transcrito aquí.)*

```c
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->chan == chan) {
      p->chan = 0;
      if(p->state == SLEEPING) {
        p->state = RUNNABLE;
      }
    }
    release(&p->lock);
  }
}
```

P6, fragmento de `wakeup()` (`kernel/proc.c`): recorre todo `proc[]` tomando cada `p->lock` y solo cambia `SLEEPING → RUNNABLE`; no llama a `yield()` ni a `sched()`.
Qué saca a un proceso `RUNNING` de la CPU hoy (buscamos todas las llamadas en `kernel/*.c`): `sched()` solo se llama desde `kexit()`, `yield()` y `sleep()`; y `yield()` solo desde `usertrap()` y `kerneltrap()` (`kernel/trap.c`, líneas 86 y 158) en interrupciones de reloj. `wakeup()` no es uno de esos eventos. Hoy A espera como máximo un tick.
1. Expropiación: A le quita la CPU a B en el siguiente tick: en cada tick se comprueba si hay un `RUNNABLE` de mayor prioridad y, si lo hay, se cede aunque el turno de B no haya terminado.

* Por qué: la regla 1 manda correr a A, y esperar al final del turno de B (el más largo, en las colas bajas) perjudica a los interactivos. Así la espera de A se acota a 1 tick (~0.1 s).
* Descartado: expropiar dentro de `wakeup()`, porque se llama desde interrupciones y con otros locks tomados (hipótesis sin verificar); y esperar al fin del turno, por la espera larga.
* Costo: A puede esperar hasta 1 tick.

2. Posición de A: en su cola superior, con `ticks_used` intacto, al final de la cola. El nivel lo decide el acumulado, no lo gastado en el último turno.

* Pendiente: XV6 no tiene orden dentro de una cola (`scheduler()` recorre `proc[]` por índice); haría falta un dato extra (hipótesis).

---

## Tutor (turno 5)

Cerramos **P6**: el fragmento confirma lo observado (recorre `proc[]`, toma cada `p->lock`, solo pasa `SLEEPING → RUNNABLE`, sin `yield()` ni `sched()`). Su búsqueda de llamadas a `sched()` también es buena evidencia, y la tomo como observada porque la reportan ustedes. Mantengo como hipótesis las líneas 86 y 158, que no he visto, y el motivo para descartar la expropiación dentro de `wakeup()` ("se llama desde interrupciones, con otros locks"), que no está respaldado por lo que mostraron.

Registro un par de cosas:

- **P7:** su regla 2 no dice nada del orden dentro de una cola. Ya señalaron que `scheduler()` recorre `proc[]` por índice, así que el orden es un problema de diseño y de XV6 a la vez.
- **P8:** dicen que B tiene "2 ticks de su turno", pero hoy XV6 hace `yield()` en cada interrupción de reloj, y ustedes observaron que cada proceso corre un tick por turno. Si el turno es más largo que un tick, algo debe cambiar en ese camino. Lo retomamos al relacionar con el código.

**Pregunta principal (regla 2 junto con la expropiación)**

Su decisión de expropiar en el siguiente tick deja un caso sin resolver. En la cola inferior están **B** y **D**, en ese orden, y B lleva 2 ticks de su turno cuando A (cola superior) lo expropia. A corre, se bloquea otra vez y la CPU queda libre.

1. ¿Quién corre ahora, B o D? ¿B conserva sus 2 ticks de turno o reinicia el turno? ¿B queda al frente o al final de su cola? Elijan y justifiquen con la regla 2.
2. Muestren con un caso de dos o tres procesos cómo su elección podría favorecer o perjudicar a D, o permitir que B gane ventaja si A lo expropia con frecuencia.

Distingan en su respuesta qué es decisión de diseño y qué sería hipótesis sobre XV6. Pista si se bloquean: comparen "B perdió la CPU porque agotó su turno" con "B la perdió sin que fuera su culpa", y piensen si la regla 2 debería tratarlos igual.

---

## Estudiantes (turno 6)

1. Quién corre: corre B, con los 2 ticks ya gastados (le queda el resto de su turno) y al frente de su cola, antes que D.
Justificación (regla 2): B no perdió la CPU por agotar su turno, sino porque la regla 1 se la quitó A. Mandarlo al final con turno reiniciado lo castigaría por algo que no fue culpa suya. Distinguimos:

* Agotó su turno: va al final y su turno se reinicia.
* Lo expropió una prioridad mayor: conserva su lugar y su turno restante.

2. Caso con B, D y E (A despierta cada ~2 ticks):

* Frente con turno reiniciado: B nunca terminaría su turno y D y E no correrían (B gana ventaja).
* Final con turno reiniciado: D correría antes y B pagaría por las interrupciones de A.
* Nuestra opción: nadie gana ni pierde por las interrupciones de A, y `ticks_used` de B sigue acumulándose para bajar cuando toque.

Decisión de diseño: B conserva frente y turno restante si lo expropió una prioridad mayor.
Hipótesis sin verificar sobre XV6:

* P7: `struct proc` no tiene campos para el orden en la cola ni para el turno en curso.
* P8: hoy `yield()` se llama en cada tick; un turno de más de un tick exige contar ticks por proceso en la rama `which_dev == 2` de `trap.c`.

---

## Tutor (turno 6)

Su elección es coherente con la regla 2 y la distinción que proponen (agotó su turno / lo expropió una prioridad mayor) es una decisión de diseño clara. Un matiz sobre su caso con B, D y E: con "frente con turno reiniciado", D y E no quedarían sin CPU indefinidamente, porque `ticks_used` de B sigue acumulándose hasta el allotment. El perjuicio sería temporal, y lo medirían en ticks hasta que B baje. Vale la pena que lo tengan presente al redactar la justificación.

**Evidencia y pendientes**

- Lo que observaron de `struct proc` es que no tiene prioridad ni tiempo de CPU usado. Que tampoco tenga campos para el orden en la cola o el turno en curso lo deducen de una lista de campos que ustedes mismos describieron como "entre otros". Sigue siendo **hipótesis** hasta que me muestren la estructura completa.
- **P7** (orden dentro de la cola) y **P8** (turno de más de un tick) quedan abiertos.
- **P9, nuevo:** definieron qué pasa con B cuando lo expropian, pero no qué pasa con el turno de un proceso que se bloquea antes de agotarlo y luego despierta. Ustedes dijeron que A queda al final de su cola con `ticks_used` intacto. ¿Su turno se reinicia? Lo dejo anotado.

**Pregunta principal (de la regla a la selección en `scheduler()`)**

Ustedes observaron que `scheduler()` recorre `proc[NPROC]` por índice y toma el primer `RUNNABLE`. Con todas las decisiones que ya tomaron (nivel, frente o final de cola, turno restante), respondan:

1. En el momento de elegir el siguiente proceso, ¿qué información necesita conocer el planificador de cada proceso `RUNNABLE`? Enuncien cada dato como una necesidad de comportamiento, no como un campo.
2. Para cada dato, ¿qué evidencia del código que ya me mostraron indica que hoy está disponible o que falta? Marquen lo que sea hipótesis.

Pista si se bloquean: tomen su caso de B y D en la cola inferior y pregúntense cómo distinguiría el recorrido actual a B (que debe ir antes) de D si ambos son `RUNNABLE`, y qué cambiaría si D tuviera un índice menor que B en `proc[]`.

Cuando respondan, adjunten, si pueden, la definición completa de `struct proc` de `kernel/proc.h`.

---

## Estudiantes (turno 7)

*(Adjunto: imagen con la definición de `struct proc`, transcrita aquí.)*

```c
struct proc {
  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state; // Process state
  void *chan;           // If non-zero, sleeping on chan
  int killed;           // If non-zero, have been killed
  int xstate;           // Exit status to be returned to parent's wait
  int pid;              // Process ID

  // wait_lock must be held when using this:
  struct proc *parent;  // Parent process

  // these are private to the process, so p->lock need not be held.
  uint64 kstack;               // Virtual address of kernel stack
  uint64 sz;                   // Size of process memory (bytes)
  pagetable_t pagetable;       // User page table
  struct trapframe *trapframe; // data page for trampoline.S
  struct context context;      // swtch() here to run process
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
};
```

`struct proc` completa: confirmamos en `kernel/proc.h` que no tiene campos de prioridad, CPU usada, turno restante ni orden en la cola. Lo "entre otros" eran `parent`, `kstack`, `sz`, `pagetable`, `trapframe`, `context`, `ofile`, `cwd` y `name`. Esto deja de ser hipótesis.
P9: si un proceso se bloquea antes de agotar su turno y despierta, su turno se reinicia y va al final de su cola. `ticks_used` se conserva, así que bloquearse no le regala allotment.
1. Qué necesita saber `scheduler()` de cada `RUNNABLE`: su nivel, su orden dentro de la cola, cuánto le queda de turno, cuánto CPU ha acumulado en el nivel y que esté `RUNNABLE`.
2. Qué hay hoy:

* Estado `RUNNABLE`: disponible (`p->state`).
* Nivel, orden, turno restante y CPU acumulada: faltan. No hay campos en `struct proc`, `yield()` se llama en cada tick y `ticks` es global del sistema.

Caso B y D: hoy gana el de menor índice en `proc[]`, no el que debe ir antes. Si D tiene menor índice que B, correría D primero. El orden dentro de la cola no sale del comportamiento actual y hay que representarlo de forma explícita.
Hipótesis sin verificar: cómo representar el orden (marca de encolado o listas) y cómo asigna `allocproc()` el índice en `proc[]`.

---

## Tutor (turno 7)

Confirmado: la imagen de `struct proc` cierra la hipótesis sobre los campos. Queda **observado** que no hay nivel, CPU acumulada, turno restante ni orden en la cola. Su lectura del caso B y D también es correcta: el recorrido actual decide por índice en `proc[]`, y ese índice no refleja lo que su regla 2 exige.

Un reparo sobre una de sus afirmaciones: dicen que "`ticks` es global del sistema" y que eso impide medir el turno por proceso. Lo que mostraron es que `clockintr()` incrementa `ticks` solo en la CPU 0. Que sea global lo sé por esa descripción, pero de ahí no se sigue por sí solo que no sirva para medir intervalos. Lo trato como pendiente, no como conclusión.

**Registros**
- **P9 cerrado como decisión de diseño:** al despertar, el turno se reinicia y el proceso va al final de su cola, con `ticks_used` conservado. Es coherente con lo que ya definieron.
- **P7 queda precisado:** el orden dentro de la cola no existe hoy y se debe representar de alguna forma. La forma concreta sigue siendo hipótesis, y no se las voy a resolver.
- **P10, nuevo:** `allocproc()` no lo han mostrado. Su afirmación sobre cómo asigna índices es hipótesis, y no necesitan resolverla ahora.

**Pregunta principal (por qué importa la regla 5 para el diseño de XV6)**

Ya identificaron qué dato falta. Ahora conecten ese dato con el *momento* en que cambia. Tomen tres de sus datos faltantes (nivel, CPU acumulada y turno restante) y, para cada uno, respondan:

1. ¿Qué evento del sistema lo modifica? Escojan entre los que ya conocen: interrupción de reloj, `yield()`, `sleep()`, `wakeup()`, `kexit()`, creación del proceso o el boost.
2. ¿Qué evidencia del código que ya me mostraron respalda que ese evento es el lugar natural donde ocurre el cambio, y cuál sería una hipótesis?

Ayúdense con esta restricción, que sí está en lo que reportaron: antes de llamar a `sched()`, el proceso debe haber cambiado su estado y `p->lock` se mantiene durante el cambio de contexto. Pregúntense qué implica eso sobre *dónde* es seguro tocar un dato del proceso.

Pista si se bloquean: tomen "CPU acumulada" y sigan un solo tick de un proceso `RUNNING`, desde la interrupción hasta que `scheduler()` vuelve a elegir. ¿En cuántos puntos distintos podría hacer falta actualizar el contador?

---

## Estudiantes (turno 8)

Corrección: `ticks` incrementado solo en la CPU 0 no impide usarlo para medir `S` (con `CPUS=1` es el reloj de esa CPU). Lo que no sirve es para un turno por proceso, porque no pertenece a ningún proceso. Queda pendiente, sin conclusión.
Qué evento modifica cada dato:

* Nivel: creación (nace arriba), tick que alcanza el allotment (baja) y boost (sube). Evidencia: `allocproc()` se llama desde dos sitios de `proc.c` (líneas 223 y 266) y los ticks pasan por `trap.c` y `clockintr()`. Hipótesis: que esas llamadas sean `userinit()` y el `fork`, y que el boost pueda hacerse desde `clockintr()`.
* CPU acumulada: cada tick que corre el proceso; se reinicia al bajar y en el boost. Evidencia: cada tick pasa por `trap.c`, donde hoy se llama a `yield()`.
* Turno restante: se descuenta en cada tick; se reinicia al agotarse (`yield()`), al despertar (`wakeup()`, P9) y al elegirse un proceso. Evidencia: `yield()` y `wakeup()` cambian el estado del proceso. Hipótesis: que reiniciarlo en `wakeup()` sea seguro.

Restricción de `sched()`: el proceso ya cambió de estado y `p->lock` se mantiene, así que los campos nuevos solo se tocan con `p->lock` tomado: dentro de `yield()`, del recorrido de `wakeup()` o de `scheduler()`.
Pista del tick: la CPU acumulada podría actualizarse en al menos cuatro puntos (`usertrap()`/`kerneltrap()`, `clockintr()`, `yield()` y `scheduler()` tras el `swtch`); actualizar en más de uno la contaría doble.
Decisión: contar en la rama de reloj de `usertrap()`/`kerneltrap()`, donde pasa cada tick de un proceso.
Hipótesis sin verificar: como `yield()` toma `p->lock` por sí mismo, contar antes exige soltar y retomar el lock o mover el conteo dentro de `yield()`.

---

## Tutor (turno 8)

Aceptada su corrección sobre `ticks`: sirve para medir `S` y no pertenece a ningún proceso, así que no sirve como turno por proceso. Queda pendiente (**P11**: qué reloj mide `S` con varias CPUs, porque solo la CPU 0 lo incrementa).

**Lo que queda respaldado, y lo que no**

- Respaldado por lo que mostraron: la restricción de `p->lock` y que los ticks pasan por la rama `which_dev == 2`.
- Hipótesis, como ustedes marcan: que las dos llamadas a `allocproc()` sean `userinit()` y `fork`, y que el boost sea seguro en `clockintr()`.
- Mis líneas 86 y 158 de `trap.c` siguen sin verificar: no he visto ese archivo.

Una inconsistencia en su respuesta: dicen que contar en la rama de reloj exige soltar y retomar el lock, pero también que los campos solo se tocan con `p->lock` tomado. Y en su tabla, "se reinicia al elegirse un proceso" no tiene evento de código que lo respalde todavía. Anoto **P12**: ¿en qué momento exacto, y con qué lock, se reinicia el turno al elegir? Lo dejo para después.

**Pregunta principal (la regla 4 con varias CPUs)**

Ustedes observaron todo con `CPUS=1`. Pero `clockintr()` incrementa `ticks` solo si `cpuid() == 0`, y cada CPU recibe su propia interrupción de reloj. Su decisión es contar la CPU acumulada en la rama de reloj de `usertrap()`/`kerneltrap()`.

1. Con 2 CPUs, y un proceso A `RUNNING` en la CPU 1, ¿qué rutina se ejecuta en cada tick de A, y se ejecuta `clockintr()` completo ahí? Distingan lo que muestra su evidencia de lo que es hipótesis.
2. Consecuencia para su diseño: si midieran el allotment con `ticks` en lugar de con el contador del proceso, ¿qué error cometerían con A en la CPU 1? ¿Y si el boost se dispara dentro de `clockintr()` (rama CPU 0), qué tendría que garantizar frente a procesos `RUNNING` en otras CPUs?

Pista si se bloquean: piensen en un proceso que corre solo en la CPU 1 mientras la CPU 0 está en `wfi`, y en qué cuenta `ticks` en ese caso.

---

## Estudiantes (turno 9)

ok, continuando:
P12: el turno se reinicia en dos momentos: cuando el proceso lo agota (en `yield()`, con `p->lock`) y cuando despierta (en `wakeup()`). En `scheduler()` solo se elige, sin tocar el turno. “Se reinicia al elegir” lo descartamos porque borraría el turno restante de un proceso expropiado.
1. Tick de A en la CPU 1: el camino es el mismo en todas las CPU: `devintr()` llama a `clockintr()` y, si devuelve 2, `usertrap()`/`kerneltrap()` llaman a `yield()`. `clockintr()` se ejecuta completo en la CPU 1, pero `ticks++` y `wakeup(&ticks)` solo ocurren si `cpuid() == 0`; en la CPU 1 solo reprograma su siguiente tick con `w_stimecmp`.

* Hipótesis: que cada CPU reciba su propia interrupción de reloj; no la probamos con `-smp 2`.

2. Medir el allotment con `ticks`: sería un error. `ticks` lo incrementa solo la CPU 0 y sus interrupciones no están alineadas con las de la CPU 1, así que el trabajo de A podría contarse como 1 o 2 ticks según el desfase, y la decisión de bajarlo se retrasaría o saltaría. Por eso usamos un contador por proceso.

* Boost en `clockintr()`: debe tomar el `p->lock` de cada proceso, incluidos los `RUNNING` en otras CPU, y la otra CPU también lo toma al contar su tick. Ya existe el orden `tickslock → p->lock` (por `wakeup(&ticks)`), y el boost debe respetarlo.
* P11: `ticks` mide `S`; la deriva entre CPU queda pendiente.

Decisión de diseño: contador por proceso bajo `p->lock`; boost bajo el `p->lock` de cada proceso; turno reiniciado en `yield()` (agotado) y `wakeup()`.
Hipótesis sin verificar: una interrupción de reloj por CPU, que `ticks` avance con la CPU 0 en `wfi`, y que el boost en `clockintr()` no cause interbloqueos.

---

## Tutor (turno 9)

Su respuesta sobre varias CPUs está bien razonada. Separaron lo observado de lo hipotético, y marcaron que no probaron `-smp 2` y que no saben si `ticks` avanza con la CPU 0 en `wfi`. El error de medir el allotment con `ticks` es plausible, pero también es hipótesis hasta que verifiquen cómo se reparten las interrupciones de reloj entre CPU.

**Qué queda respaldado y qué no**

- **Respaldado por lo que mostraron al inicio:** `clockintr()` incrementa `ticks` solo si `cpuid() == 0`, llama a `wakeup(&ticks)` y reprograma con `w_stimecmp`.
- **Todavía hipótesis, aunque lo enuncian como hecho:**
  - Que `devintr()` llame a `clockintr()` y devuelva 2. Nunca mostraron `devintr()`.
  - Que ya exista el orden `tickslock → p->lock`. Mostraron que `clockintr()` llama a `wakeup(&ticks)`, pero no que `tickslock` esté tomado en ese momento.

**Registros**
- **P11 se precisa:** la deriva entre CPU al medir `S` con `ticks` sigue abierta.
- **P12 se reabre en parte:** descartaron bien "reiniciar al elegir", pero dicen que el turno se reinicia en `yield()` cuando se agota. Eso me lleva a la pregunta de abajo.
- **P13, nuevo:** el orden `tickslock → p->lock`. Hipótesis hasta ver `clockintr()` completo.
- **P14, nuevo:** ¿con qué valor de turno nace un proceso, y qué pasa con el turno al bajar de nivel o en el boost? No lo han definido.
- **P15, nuevo:** buscaron todas las llamadas a `sched()`, pero ¿es `wakeup()` el único sitio que pone un proceso en `RUNNABLE`? No lo han comprobado.

**Pregunta principal (cuando varias reglas coinciden en un mismo tick)**

Ustedes decidieron que un proceso expropiado conserva su turno restante y su lugar, y que uno que agota su turno se reinicia y va al final. También decidieron que al alcanzar el allotment baja de nivel. Hoy todos estos casos terminan en la misma rama de reloj, que llama a `yield()`.

Consideren **B**, en la cola de nivel 2, con `ticks_used = 9` y 2 ticks de un turno de 3. En un solo tick ocurre que `ticks_used` llega a 10 y **A** (nivel 0) acaba de volverse `RUNNABLE`.

1. Con sus reglas, ¿qué pasa con B? Indiquen su nivel, su posición, su turno y su `ticks_used` después del tick. Si dos reglas chocan, digan cuál tiene precedencia y por qué.
2. Sin pensar todavía en campos ni en código: ¿qué información necesita tener a mano el código que ejecuta ese tick para decidir entre "agotó el turno", "lo expropiaron" y "bajó de nivel"? ¿Está esa información disponible hoy en el punto donde se llama a `yield()`? Respondan con la evidencia que ya mostraron.

Pista si se bloquean: resuelvan primero cada caso por separado, con una sola condición cumplida, y luego combínenlos.

**Evidencia que les pido**

Adjunten, si pueden, de `kernel/trap.c`: `devintr()`, `clockintr()` completo y las ramas `which_dev == 2` de `usertrap()` y `kerneltrap()`. Con eso podremos cerrar varias hipótesis de una vez. Mis referencias a las líneas 86 y 158 siguen sin verificar.

---

## Estudiantes (turno 10)

*(Adjunto: el archivo `HOMEWORK-Scheduling_v3.md`, con el enunciado del taller "Exploración de la planificación de procesos en XV6". No se reproduce aquí su contenido.)*

mira el enunciado del taller, que incluye el link al repositorio oficial de xv6 y mas herramientas para que puedas observar las evidencias. Ahora respondiendo a las preguntas:

P13 (cerrado): `clockintr()` llama a `wakeup(&ticks)` entre `acquire(&tickslock)` y `release(&tickslock)`, así que el orden `tickslock → p->lock` existe. `devintr()` llama a `clockintr()` y devuelve 2 cuando `scause == 0x8000000000000005L`.
P14 (decidido): un proceso nace con el turno completo de la cola más alta; al bajar de nivel recibe el turno completo de la nueva cola y `ticks_used = 0`; en el boost, turno completo de la cola máxima.
P15 (comprobado con `grep`): `wakeup()` no es el único sitio que pone `RUNNABLE`: también `proc.c` líneas 228 y 301 (creación y `fork`), 505 (`yield()`), 590 (`wakeup()`) y 614 (parece `kill`, por confirmar).
Caso de B (coinciden turno agotado, allotment alcanzado y A `RUNNABLE`):

* Resultado: B baja de nivel, va al final de la cola inferior, con turno completo y `ticks_used = 0`. A corre después por la regla 1.
* Precedencia: allotment > turno agotado > expropiación. El allotment decide la cola; el turno y la expropiación solo importan dentro de una cola, y B ya la dejó.

Información que necesita el tick: CPU acumulada y allotment del nivel, turno restante, si hay un `RUNNABLE` de mayor prioridad, y el nivel y la posición de B.
No está disponible hoy: la rama de reloj solo sabe `which_dev == 2`; `yield(void)` solo conoce `myproc()`; `struct proc` no tiene esos datos, y `yield()` no mira a otros procesos ni sabe por qué cede.
Decisión de diseño: precedencia allotment > turno > expropiación.
Hipótesis sin verificar: que recorrer `proc[]` desde la rama de reloj sea seguro, y que la línea 614 sea de `kill`.

---

## Tutor (turno 10)

**Qué queda cerrado y qué no**

- **P13 cerrado, según ustedes:** `clockintr()` llama a `wakeup(&ticks)` entre `acquire` y `release` de `tickslock`. Eso implica el orden `tickslock → p->lock`. `devintr()` devuelve 2 con `scause == 0x8000000000000005L`.
- **P14 cerrado como decisión de diseño:** turno completo al nacer, al bajar y en el boost.
- **P15 abierto en parte:** encontraron cinco sitios que ponen un proceso en `RUNNABLE`, y eso es un hallazgo útil. Siguen siendo hipótesis que 228 y 301 sean creación y `fork`, y que 614 sea `kill`. Anoto **P16**: su decisión "al despertar, turno reiniciado y al final de la cola" se formuló pensando en `wakeup()`. Si `kill` también despierta procesos, ¿esa decisión cubre ese camino?
- **Precedencia allotment > turno > expropiación:** su razonamiento es coherente. B ya dejó la cola, así que lo que valía dentro de ella deja de importar.
- **P12 reabierto, por una inconsistencia:** dicen que el turno agotado se reinicia en `yield()`. Pero también dicen que `yield(void)` solo conoce `myproc()` y no sabe por qué cede. Si `yield()` se llama tanto por turno agotado como por expropiación, y las dos decisiones tienen efectos distintos en su diseño, alguien tiene que distinguirlas antes. Todavía no han dicho quién. Lo dejo anotado para después.

**Pendiente que encontré en su caso de B:** en mi enunciado puse a B en el "nivel 2", pero ustedes nunca decidieron cuántos niveles hay. Tampoco qué pasa cuando un proceso alcanza el allotment en el nivel más bajo, donde no hay cola inferior. Es **P17**, y es la primera duda de su descripción original, que sigue sin respuesta.

**Pregunta principal (parámetros)**

Sin esos valores no pueden escribir la descripción revisada ni escenarios concretos.

1. Propongan el número de niveles, el turno de cada nivel, el allotment de cada nivel y `S`, todo en ticks. Justifiquen cada valor con dos datos: lo que observaron del tick (~0.1 s, según el comentario del código) y los casos que ya resolvieron (A con 9 de cada 10 ticks, C con 3 de cada 10 y el proceso interactivo con 1 de cada 10).
2. Pruébenlos con un proceso **P** que solo calcula y nace en el nivel más alto. Respondan en qué tick llega al nivel más bajo y qué le pasa ahí cuando alcanza el allotment. Luego respondan cuánto tiempo, como máximo, puede pasar sin CPU un proceso en el nivel más bajo si siempre hay otro proceso listo en un nivel superior, y si eso es aceptable con su `S`.

Si un turno dura más de un tick, eso retoma el pendiente **P8**, así que digan qué valor eligen y qué implica para el camino del reloj. Si se bloquean, empiecen con solo dos niveles y valores pequeños, resuelvan P a mano y luego generalicen.

**Evidencia para después:** cuando lleguemos a P12, necesitaré ver `yield()` completo y la rama `which_dev == 2` de `usertrap()` y `kerneltrap()`. Pueden ir copiándolos desde ahora.

---

## Estudiantes (turno 11)

*(Adjunto: imagen con código de `yield()`, `usertrap()` y `kerneltrap()`, transcrito aquí.)*

```c
// kernel/proc.c
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// kernel/trap.c, usertrap()
  if(which_dev == 2)
    yield();

// kernel/trap.c, kerneltrap()
  if(which_dev == 2 && myproc() != 0)
    yield();
```

P16: toda transición SLEEPING → RUNNABLE (por wakeup() o por kill) recibe el mismo trato: turno reiniciado, final de su cola y ticks_used intacto. Hipótesis: que la línea 614 sea de kill.

P12: quien distingue el motivo es la rama de reloj de usertrap()/kerneltrap(), donde se evalúa la precedencia (allotment, turno agotado, expropiación) y se fija nivel, posición y turno. yield() solo aplica el cambio de estado con p->lock. Sin resolver: cómo se lo comunica.

P17: 3 niveles. El allotment solo existe en los niveles 0 y 1; en el nivel 2 no hay descenso.

1. Parámetros (1 tick ≈ 0.1 s)

| Nivel | Turno | Allotment |
| --- | --- | --- |
| 0 | 1 tick | 10 ticks |
| 1 | 2 ticks | 10 ticks |
| 2 | 4 ticks | no aplica |

S = 50 ticks.

Turno: el nivel 0 coincide con XV6 hoy; los niveles bajos usan turnos más largos para hacer menos cambios de contexto.
Allotment 10: A baja en unos 11 ticks, C en ≈33 (antes del boost).
S = 50: menor que los ≈100 ticks que tardaría el interactivo en acumular 10, así que el boost lo reinicia antes de que baje.

2. Caso de P: baja al nivel 1 en el tick 10 y al nivel 2 en el tick 20, y ahí se queda hasta el boost. Espera máxima sin CPU: hasta el siguiente boost (S = 50 ticks ≈ 5 s) más su turno en el nivel 0. Es aceptable porque son procesos intensivos en CPU y sin urgencia. No lo hemos probado.

P8: los turnos de 2 y 4 ticks exigen contar ticks de turno por proceso y llamar a yield() solo cuando corresponda.

Hipótesis sin verificar: que 614 sea kill y la espera máxima de ≈5 s.

---

## Tutor (turno 11)

Su captura confirma lo que describieron: `yield(void)` solo toma `myproc()`, adquiere `p->lock`, pone `RUNNABLE` y llama a `sched()`. Además, las dos ramas de reloj lo llaman sin condición. Queda como observado, con la salvedad de que no he visto esos archivos y solo me baso en su imagen.

**Cierres y decisiones**

- **P16 cerrado como decisión de diseño:** el mismo trato para toda transición `SLEEPING → RUNNABLE`. Que la línea 614 sea `kill` sigue siendo hipótesis.
- **P17 cerrado como decisión de diseño:** 3 niveles, con allotment solo en los niveles 0 y 1.
- **P12 avanza, no se cierra:** ubicar la decisión en la rama de reloj es coherente con la captura, porque ahí se evalúa antes de llamar a `yield()`. Pero esa rama no toma ningún lock, y ustedes mismos dijeron que los campos nuevos solo se tocan con `p->lock`. De eso sale la pregunta de abajo.

**Pendientes nuevos sobre sus parámetros**

La tabla es razonable y bien justificada (el turno de 1 tick en el nivel 0 coincide con XV6 hoy). Hay tres cosas por revisar:

- **P18:** dicen que la espera máxima en el nivel 2 es "hasta el boost más su turno en el nivel 0". Eso supone que no hay nadie delante en la cola 0. Además, no definieron en qué posición de la cola 0 entran los procesos tras un boost. Hasta que lo decidan, no pueden sostener "≈5 s".
- **P19:** con allotment 10 y `S = 50`, un proceso que use más de ~20 % de la CPU baja antes del siguiente boost. ¿Es ese su criterio de "interactivo"? Aparte, el "C en ≈33 ticks" depende de cuánto dura su E/S, y nunca lo fijaron.
- **P20:** su contador cuenta las veces que la interrupción de reloj cae sobre el proceso. ¿Es eso equivalente a CPU consumida? Es una hipótesis que conviene registrar, porque reabre en parte P1.

Todas estas son candidatas para la sección "decisiones que necesitan soporte adicional" que pide el enunciado. No las resuelvan ahora.

**Pregunta principal (quién sostiene qué lock al decidir)**

Con su captura delante:

1. En la rama `which_dev == 2`, ¿qué lock tiene la CPU en el momento en que evalúa "allotment, turno agotado, expropiación"? ¿Y en qué momento toma `yield()` el `p->lock`? Tomando esa respuesta como punto de partida, indiquen qué datos de **otros** procesos necesita leer su decisión (por ejemplo, si hay un `RUNNABLE` de nivel superior) y qué lock exigiría leerlos, según la regla que ya aceptaron para `struct proc`.
2. Piensen en dos CPU ejecutando esa rama a la vez, cada una con un proceso `RUNNING` distinto. ¿Qué podría salir mal según cómo organicen esos locks? Si no lo ven, empiecen por escribir paso a paso qué toma cada CPU y en qué orden.

Distingan, como siempre, lo observado de lo hipotético. En particular, digan si el `myproc() != 0` de `kerneltrap()` afecta a su conteo de ticks.

Después de esta respuesta pasamos a los escenarios de comprobación y luego a sus conclusiones, para que lleguen a cubrir los objetivos A, B y C.

---

## Estudiantes (turno 12)

P18, P19 y P20: quedan registradas como decisiones que necesitan soporte adicional, sin resolverlas ahora:

* P18: posición del proceso en la cola 0 tras un boost, y por tanto la espera real en el nivel 2.
* P19: qué consideramos “interactivo” con allotment 10 y `S` = 50 (más o menos el 20 % de CPU), y cuánto dura la E/S de C.
* P20: que contar interrupciones de reloj sobre el proceso sea equivalente a CPU consumida (es un muestreo; reabre en parte P1).

1. Qué lock tiene la CPU al evaluar en la rama `which_dev == 2`

* Observado: en `usertrap()` no se toma ningún lock entre la entrada y la rama `if (which_dev == 2) yield();` (solo `myproc()`, que hace `push_off`/`pop_off`, y `devintr()`); en `kerneltrap()` pasa lo mismo, y además hace `panic("kerneltrap: interrupts enabled")` si las interrupciones están activadas. Como `acquire()` llama a `push_off()` para desactivar interrupciones, una interrupción de reloj no puede llegar con un spinlock tomado (esto último es deducción de ese código).
* `yield()` toma `p->lock` por su cuenta (`acquire(&p->lock)`), antes de poner `RUNNABLE` y llamar a `sched()`.
* Consecuencia (observada en `acquire()`): si la rama tomara `p->lock` y luego llamara a `yield()`, `acquire` haría `panic("acquire")`, porque `holding(lk)` detecta que la misma CPU ya lo tiene. Hay que soltarlo antes de `yield()` o mover el conteo dentro de `yield()` (o de una función nueva que tome el lock una sola vez). Aún sin decidir.
* Datos de otros procesos: para saber si hay un `RUNNABLE` de nivel superior hay que leer `state` y nivel de cada otro proceso. Por la regla que aceptamos para `struct proc`, eso exige tomar el `p->lock` de cada proceso que se lea. Observado: `scheduler()` y `wakeup()` toman un solo `p->lock` por iteración y lo sueltan antes del siguiente.

2. Dos CPU en esa rama a la vez, con procesos `RUNNING` distintos (P1 y P2)

* Interbloqueo posible: si cada CPU mantuviera el lock de su propio proceso mientras toma los de los demás, P1 tomaría `P1.lock` y esperaría `P2.lock`, y P2 haría lo contrario. Sería un interbloqueo clásico.
* Decisión: nunca mantener dos `p->lock` a la vez. Cada CPU actualiza los contadores de su proceso bajo su lock, lo suelta, recorre a los demás tomando un lock a la vez (como `wakeup()`) y luego llama a `yield()`.
* Costo: los datos leídos pueden haber cambiado cuando se actúe, y las dos CPU podrían ceder por el mismo `RUNNABLE` de mayor prioridad. Solo una lo elegirá en `scheduler()` (la otra verá `RUNNING`), así que el efecto es un cambio de contexto de más, sin pérdida de corrección.
* Hipótesis: no lo probamos con `-smp 2`.

`myproc() != 0` en `kerneltrap()`

* Observado: el `yield()` de `kerneltrap()` solo ocurre si `myproc() != 0`; si la CPU está en `scheduler()` (`c->proc = 0`) y llega un tick, no hay proceso al que cobrárselo.
* Efecto en el conteo: si el conteo va en esa rama, debe ir dentro de la misma condición, o se desreferenciaría un puntero nulo. Los ticks en que la CPU está ociosa no se cuentan, lo cual es correcto. Pero un tick que cae mientras un proceso ejecuta código del kernel (por ejemplo dentro de una llamada al sistema) entra por `kerneltrap()` con `myproc() != 0`, así que hay que contar en ambas ramas (`usertrap()` y `kerneltrap()`). No hay doble conteo, porque un tick entra por un solo manejador. Esto responde el punto 3 de 4.5 (ticks en modo kernel).

Decisiones de diseño: nunca dos `p->lock` a la vez; contar en ambas ramas. Hipótesis sin verificar: el comportamiento con dos CPU, y que la deducción sobre `push_off` cubra todos los casos (por ejemplo, locks que no son spinlocks).

---

## Tutor (turno 12)

Su análisis de los locks es coherente: el `panic` de `acquire` si la misma CPU ya tiene el lock, la separación entre actualizar lo propio y leer lo ajeno, y la regla de no sostener dos `p->lock` a la vez. También entendieron bien por qué contar en ambas ramas no duplica el conteo. Estos detalles vienen de su lectura del código: en la captura solo vi `yield()` y las dos ramas de reloj, así que que `usertrap()` no tome locks, el `panic` de `kerneltrap()` y el de `acquire()` no los he visto. Los tomo como observados por ustedes y deben ir en el informe con el fragmento que los muestre.

**Registros**
- **P18, P19 y P20:** quedan en su lista de decisiones que necesitan soporte adicional.
- **P21, nuevo:** su costo (cambio de contexto de más) omite el recorrido de `proc[]` con un lock por proceso en cada tick de cada CPU. No tiene por qué ser un problema, pero hoy es hipótesis.
- **P22, nuevo:** "nunca dos `p->lock` a la vez" es una regla suya. ¿El código existente ya la cumple? Eso lo tendrían que comprobar en `proc.c`, y también es hipótesis hoy.
- **P23, nuevo:** escriben "esto responde el punto 3 de 4.5", y esa numeración no aparece en nuestra conversación ni en el enunciado que adjuntaron. Si viene de otra fuente, indíquenla en el informe. Si no, aclaren a qué se refiere.

**Pregunta principal (objetivo C: escenarios de comprobación)**

Ya tienen reglas y parámetros (3 niveles, turnos 1/2/4, allotment 10 en los niveles 0 y 1, `S = 50`). Ahora deben describir cómo comprobarían una implementación futura. Escriban un escenario para su decisión sobre la regla 4 que habría fallado con la regla original (quantum por turno). Pueden usar el proceso A de 9 ticks de cada 10 con E/S breve entre medias. El escenario tiene que incluir:

1. Qué procesos participan y qué hace cada uno, con números.
2. Qué regla se comprueba.
3. Qué comportamiento esperan ver, en ticks y en cola, si la implementación es correcta.
4. Qué observación indicaría que está mal.

Tengan en cuenta que `S = 50` ocurre en la misma ventana. ¿Cuándo baja A y cuándo lo sube el boost? Elijan la ventana de observación para que el resultado sea concluyente. El escenario se debe poder describir con una sola CPU y con lo que alguien vería desde fuera de XV6.

Pista si se bloquean: calculen primero, a mano, la línea de tiempo de A durante 60 ticks con sus parámetros. Después decidan qué momento de esa línea separa su regla de la regla original.

Cuando terminen, seguimos con un segundo escenario y con las conclusiones. Para el informe, también vayan marcando de tres a cinco intercambios que les parezcan significativos de esta conversación.

---

## Estudiantes (turno 13)

P21 a P23: P21 y P22 quedan como hipótesis por verificar (el costo del recorrido de proc[] por tick y que el código actual ya cumpla “nunca dos p->lock a la vez”). El “punto 3 de 4.5” se refiere a nuestro reporte, no al enunciado.

Escenario 1: regla 4 con allotment acumulado

1. Procesos (CPUS=1; niveles 0/1/2, turnos 1/2/4, allotment 10 en los niveles 0 y 1, S = 50):

A: único proceso de usuario. Ráfagas de 1 tick (menores que el turno de 2 del nivel 1) con E/S breve entre medias. Para calcular a mano: usa la CPU 9 de cada 10 ticks (el tick 9 de cada decena lo pierde en E/S).
Observador: traza del kernel con el tick, el pid y el cambio de nivel. Ventana: ticks 0 a 59.

2. Regla: la regla 4 (CPU acumulada por nivel aunque ceda antes) y la regla 5 (boost).

3. Comportamiento esperado:

Tick 11: baja al nivel 1 (ticks_used llega a 10 al final del tick 10).
Tick 22: baja al nivel 2.
Tick 50: el boost lo sube al nivel 0 con ticks_used = 0.

| Ticks | Regla propuesta | Regla original |
| --- | --- | --- |
| 1–10 | nivel 0 | nivel 1 |
| 11–21 | nivel 1 | nivel 1 |
| 22–49 | nivel 2 | nivel 1 (nunca baja) |
| 50–59 | nivel 0 | nivel 1 desde el 51 |

El momento que separa las reglas es el tick 22: solo la propuesta saca a A del nivel 1, aunque usa ≈90 % de la CPU.

4. Señales de error (±1 tick):

A sigue en el nivel 1 en el tick 30: cuenta el quantum por turno o reinicia el contador en cada E/S.
A baja antes del tick 10: cuenta tiempo transcurrido y no CPU.
A baja al nivel 2 hacia el tick 12: ticks_used no se reinicia al cambiar de nivel.
A no está en el nivel 0 entre los ticks 50 y 59: falta el boost.

Supuestos: el modelo de 9 de cada 10 ticks es una simplificación. Hipótesis: que la E/S real de XV6 se comporte así y que contar interrupciones de reloj (P20) reproduzca el cálculo.

---

## Tutor (turno 13)

Entendido, vamos a cerrar. Su escenario 1 tiene buena estructura: procesos, regla, comportamiento esperado, señales de error y una ventana elegida con sentido. Antes de pegarlo en el informe, revisen tres cosas. Las planteo como preguntas y no las corrijo.

1. En su tabla, ¿en qué nivel estaría A durante los ticks 1 a 10 con la regla original? Fíjense en qué nivel dice la columna "Regla original" y en qué nivel dice su propio texto ("nunca baja").
2. Las ráfagas de A son de 1 tick y el turno del nivel 0 también es de 1 tick. ¿Gasta A "todo su quantum" en el nivel 0 con la regla original? Si es así, el escenario no separa las reglas donde ustedes creen. Piensen qué ráfaga o qué turno haría que sí lo separe.
3. Escriben que `ticks_used` llega a 10 "al final del tick 10", pero A usa la CPU 9 de cada 10 ticks, y su tabla da el tick 11. ¿Cuál es el tick correcto?

**Registros finales**

- **P21 y P22:** hipótesis por verificar.
- **P23:** resuelto, la numeración es de su reporte.
- Lo que pasó con sus parámetros (P18, P19 y P20) y con el boost en `clockintr()` (P13) se queda como "decisiones que necesitan soporte adicional" en el informe.
- Si pueden, describan un segundo escenario con el mismo formato de cuatro partes. Su decisión sobre B y D con la expropiación (regla 2) o la del boost sobre un proceso `SLEEPING` son buenos candidatos.

**Tarea de cierre: sus conclusiones**

Escriban sus conclusiones con sus propias palabras, de forma breve y sin pegar el historial. Cubran la descripción revisada de la variante de MLFQ (objetivo A), los cambios que necesitaría XV6 con su propósito y la evidencia del código (objetivo B), los escenarios que proponen (objetivo C) y los pendientes de investigación. Yo no voy a redactar el reporte por ustedes.

**Última pregunta (comprobación de consistencia)**

Cuando tengan sus conclusiones, resuelvan este caso **usando solo lo que dicen en ellas**, y no lo que recuerden de la conversación. Una CPU, 3 niveles, turnos 1/2/4, allotment 10, `S = 50`. En el tick 48, **X** (solo calcula) está en el nivel 2 con 1 de 4 ticks de su turno. **Y** estaba `SLEEPING` en el nivel 0 con `ticks_used = 9`, y se despierta en ese mismo tick.

1. Describan los ticks 48 a 52: quién corre en cada uno, en qué nivel y posición está cada proceso, y cuánto le queda de turno y de `ticks_used`. Para cada paso, indiquen qué regla de sus conclusiones lo decide.
2. Señalen si hay algún paso que sus conclusiones **no** deciden. Si lo hay, anótenlo como pendiente en vez de inventar una regla.

---

## Estudiantes (turno 14)

1. Nivel de A en los ticks 1 a 10 con la regla original: nivel 1. La tabla estaba bien, pero “nunca baja” está mal escrito: debe decir “no baja del nivel 1, nunca llega al 2”. A baja del 0 al 1 en el tick 1 y se queda en el 1.

2. ¿Gasta A todo su quantum en el nivel 0? Sí: su ráfaga de 1 tick es igual al turno de 1 tick, así que la regla original la baja en el tick 1. Eso se debe a agotar el turno, no a ceder antes, y por eso en los ticks 1 a 10 la diferencia entre las reglas no prueba la trampa. Donde sí se prueba es en el nivel 1, donde la ráfaga (1) es menor que el turno (2). Corregimos la ventana concluyente: del tick 11 al 49. En el tick 11 ambas reglas ponen a A en el nivel 1, y solo la nuestra la baja al nivel 2 en el tick 22.

3. Tick correcto: no hay contradicción, hay una convención sin explicar. Llamamos “tick n” al intervalo [n, n+1). A acumula 10 ticks de CPU en los ticks 0 a 8 y 10 (el 9 es E/S), así que ticks_used llega a 10 al final del tick 10, y A empieza el tick 11 en el nivel 1. Lo escribimos así en el informe: “baja al final del tick 10 / está en el nivel 1 desde el tick 11”. Igual con la segunda bajada: final del tick 21, nivel 2 desde el tick 22.

Escenario 2: expropiación y orden dentro de la cola

Procesos (CPUS=1): B y D en el nivel 2 (B delante de D). B lleva 2 de 4 ticks de su turno al empezar el tick 20 y corre ese tick (le queda 1). A, del nivel 0, está SLEEPING y despierta durante el tick 20; al correr, usa 1 tick y vuelve a bloquearse. Ventana: ticks 20 a 27 (antes del boost del 50).
Regla: reglas 1 y 2 (prioridad y orden dentro de la cola).
Esperado: A corre en el tick 21 (se expropia a B al final del tick 20). B vuelve a correr en el tick 22 con su último tick de turno y luego va al final de la cola. D corre en los ticks 23 a 26 y B vuelve en el 27.
Mal si: D corre en el tick 22 (B fue al final al ser expropiado), B corre del 22 al 25 (turno reiniciado) o A espera hasta el tick 23 o más (no hay expropiación en el siguiente tick).

Conclusiones

MLFQ con 3 niveles (turnos de 1, 2 y 4 ticks, allotment 10 en los niveles 0 y 1, S = 50). El allotment se acumula por nivel aunque el proceso ceda antes; baja en el tick en que llega a 10. El turno y el allotment son distintos. Un proceso expropiado por una prioridad mayor conserva el frente y el resto de su turno; uno que agota su turno va al final con turno nuevo. Al despertar conserva nivel y ticks_used, con turno nuevo y al final de su cola. El boost incluye a los SLEEPING y reinicia nivel, ticks_used y turno. Precedencia: allotment, luego turno agotado, luego expropiación.

struct proc no tiene nivel, CPU acumulada, turno ni orden; scheduler() elige por índice de proc[]; yield() se llama en cada tick sin saber el motivo. Hay que añadir esos datos, contar el tick en la rama de reloj de usertrap() y kerneltrap() (bajo p->lock, sin dos locks a la vez), aplicar el trato al despertar en todo sitio que pone RUNNABLE, y añadir un boost cada S ticks desde clockintr().

E1 comprueba la regla 4 con A (ventana tick 11–49, separa en el tick 22) y E2 comprueba las reglas 1 y 2 con B, D y A.

Pendientes: P4, P5, P7, P8, P10, P11, P12, P14 a P22, y los valores de los parámetros.

Caso de los ticks 48 a 52 (X en nivel 2 con 3 de 4 ticks de turno por usar al empezar el 48, Y despierta en el 48 con ticks_used = 9; Y solo calcula una vez despierto, supuesto nuestro):

Tick 48: corre X (nivel 2; termina con turno restante 2). Y pasa a RUNNABLE en el nivel 0, ticks_used = 9, turno completo (1) y al final de su cola. (Reglas de despertar y regla 3.)
Fin del tick 48: Y tiene mayor prioridad, así que X es expropiado: conserva el frente del nivel 2 y su turno restante (2). (Regla 1.)
Tick 49: corre Y; al final llega a ticks_used = 10 y a turno agotado (1 de 1). Por la precedencia, baja al nivel 1, final de la cola, turno 2, ticks_used = 0. (Regla 4.)
Ticks 50 a 52: el boost deja a X e Y en el nivel 0 con ticks_used = 0 y turno completo (1 tick); se alternan cada tick. (Regla 5.)

Lo que nuestras conclusiones no deciden:

Orden del boost y del conteo en el mismo tick: en el tick 49→50, clockintr() (boost) se ejecuta dentro de devintr() antes de la rama que cuenta ticks. Si el boost va primero, Y llegaría al descenso con ticks_used = 0 y no con 10. No lo decidimos.
Posición en la cola 0 tras el boost (P7, P18): no sabemos quién corre primero en el tick 50, X o Y.
Convención del tick del boost: si ocurre al terminar el tick 49 o al empezar el 50.

---

## Tutor (turno 14)

Su última respuesta es consistente en lo esencial. En el caso de los ticks 48 a 52 aplicaron sus propias reglas sin recurrir a lo que recordaban de la conversación y, sobre todo, encontraron tres huecos reales. Esos huecos son lo mejor de ese caso, porque muestran que sus conclusiones no deciden el orden del boost y del conteo en el mismo tick, la posición en la cola 0 tras el boost y la convención del tick del boost. Si el boost va antes del conteo, Y no llega a 10 en el tick 49, y eso cambia todo el desenlace. Pónganlo en la sección de decisiones que necesitan soporte adicional.

**Su corrección de la tabla del escenario 1.** La acepto: la ventana 11 a 49 y la convención "[n, n+1)" resuelven mis tres preguntas. Queda una duda sobre el modelo de A. Dicen que A tiene "ráfagas de 1 tick con E/S entre medias" y también que usa la CPU 9 de cada 10 ticks, con un solo tick perdido por decena. Si hay una E/S después de cada ráfaga, ¿cuántas E/S cabrían en una decena de ticks? ¿Encaja con una sola? Es el pendiente P19 (cuánto dura la E/S), y conviene resolverlo o declararlo antes de entregar.

**Escenario 2.** La línea de tiempo es coherente con sus reglas 1 y 2, y las señales de error son concretas y distinguen cada decisión. Díganlo en el informe como lo que es: describe cómo debería comportarse la implementación, sin haberla ejecutado.

**Observaciones sobre sus conclusiones, como preguntas**

- Dicen que `scheduler()` elige por índice de `proc[]`, y eso lo identificaron como el problema. Pero en la lista de cambios no aparece `scheduler()`. ¿Qué necesita cambiar ahí para que su regla 2 se cumpla?
- El enunciado pide para cada cambio su propósito, qué regla lo requiere y qué evidencia del código lo respalda. Su párrafo mezcla los cambios. ¿Podrían separarlos y completar cada uno en esos tres campos? Esa tabla la escriben ustedes.
- Escriben "añadir un boost cada S ticks desde `clockintr()`" como cambio. Pero seguridad del boost ahí, interbloqueos y el orden con el conteo siguen siendo hipótesis, y así deben quedar marcadas.
- Su lista de pendientes dice "P14 a P22". Sin embargo, P14, P16 y P17 las dejaron como decisiones de diseño, y P13 la cerraron con evidencia. ¿Cuáles están realmente abiertas?
- En el caso de los ticks 48 a 52, para el despertar de Y citan la regla 3 ("el proceso nuevo entra en la cola más alta"). ¿Aplica a un proceso que despierta, o es otra de sus decisiones?

---

## Estudiantes (turno 15)

puedes exportar/copiar todo lo que llevamos en este chat hasta ahora (lo que yo te mando y lo que tu me mandas) en un md
