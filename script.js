const CELL_EMOJI = {
    0: "⬜️", 1: "🪙", 2: "🍖", 3: "🌲", 4: "🏠",
    5: "🏰", 6: "🚩", 7: "🛖", 99: "👨‍🌾"
};

let currentStrategy = null;
let tickData = [];
let currentTick = 0;
let animTimer = null;
const TICK_DELAY = 500;

function selectStrategy(id) {
    currentStrategy = `strategy${id}`;
    document.getElementById("strategy-label").textContent = `Selected: ${currentStrategy}`;
    loadAllTicks();
}

async function loadAllTicks() {
    tickData = [];
    let index = 0;
    let consecutiveFails = 0;

    while (consecutiveFails < 3) { // permite 3 fallos antes de parar
        const padded = index.toString().padStart(3, '0');
        const url = `../output/simulation/${currentStrategy}/tick_${padded}.json`;

        try {
            const resp = await fetch(url);
            if (!resp.ok) {
                consecutiveFails++;
                index++;
                continue;
            }
            const json = await resp.json();
            tickData.push(json);
            index++;
            consecutiveFails = 0; // reset al encontrar uno válido
        } catch (err) {
            console.log("Fetch error:", err);
            break;
        }
    }

    currentTick = 0;
    renderTick(0);
    startAnim();
    console.log(`✅ Loaded ${tickData.length} ticks`);

}


async function renderTick(index) {
    if (!tickData[index]) return;

    await updateMap(index);           // Asegura que el mapa se cargue antes de continuar
    updateInfoPanel(index);
}




async function loadTickMap(strategy, tickIndex) {
    const padded = tickIndex.toString().padStart(3, '0');
    const path = `output/ticks/${strategy}/tick_${padded}.txt`;

    try {
        const response = await fetch(path);
        if (!response.ok) {
            console.warn("Could not load map txt:", path);
            return null;
        }

        const text = await response.text();
        const rows = text.trim().split('\n');
        const map = rows.map(row => {
            return row.trim().split(/\s+/).map(cell => {
                if (cell === "V") return 99;
                return parseInt(cell);
            });
        });
        return map;

    } catch (err) {
        console.error("Error loading tick map:", err);
        return null;
    }
}

async function updateMap(tickIndex) {
    const strategy = currentStrategy;
    const map = await loadTickMap(strategy, tickIndex);

    let output = "";

    if (!map) {
        output = "⚠️ Map not found for tick " + tickIndex;
    } else {
        for (let y = 0; y < map.length; y++) {
            for (let x = 0; x < map[y].length; x++) {
                const val = map[y][x];
                output += CELL_EMOJI[val] || "❓";
            }
            output += "\n";
        }
    }

    document.getElementById("map-area").textContent = output;
}


function updateInfoPanel(tickIndex) {
    const tick = tickData[tickIndex];
    if (!tick) return;

    document.getElementById("tick-counter").textContent = `Tick = ${tick.tick}`;

    const rightInfo = document.getElementById("right-info");
    rightInfo.innerHTML = `👨‍🌾 x ${tick.villagers.length} | 🌲 ${tick.resources.wood} 🪙 ${tick.resources.gold} 🍖 ${tick.resources.food}<br>`;
    rightInfo.innerHTML += `Effort: ${tick.statistics.collectionEffort} | Dist: ${tick.statistics.totalDistance}<br>`;

    tick.villagers.forEach(v => {
        if (v.actions.length > 0) {
            rightInfo.innerHTML += `V${v.id}: ${v.actions[0]}<br>`;
        }
    });
}


function startAnim() {
    console.log("🟢 startAnim called");

    if (animTimer !== null || tickData.length <= 1) {
        console.log("⚠️ Not enough ticks to animate or animation already running");
        return;
    }

    // ✅ Hacemos async la función step
    async function step() {
        if (currentTick >= tickData.length) {
            console.log("✅ Animation finished");
            animTimer = null;
            return;
        }

        console.log(`🔄 Rendering tick ${currentTick}`);
        await renderTick(currentTick++); // ✅ Espera a que termine antes de continuar
        animTimer = setTimeout(() => step(), TICK_DELAY);
    }

    step(); // Lanza el primer paso
}


