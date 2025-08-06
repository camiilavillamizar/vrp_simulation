const CELL_EMOJI = {
    0: "⬜️",     // Empty
    1: "🪙",     // Gold
    2: "🍖",     // Food
    3: "🌲",     // Wood
    4: "🏠",     // Dropoff
    5: "🏰",     // Town Center
    6: "🚩",     // Enemy
    7: "🛖",     // Own Building
    10: "🟫",    // Wood agotado
    11: "🟥",    // Food agotado
    12: "🟨",    // Gold agotado
    99: "👨‍🌾"    // Villager
};

let currentStrategy = null;
let tickData = [];
let currentTick = 0;
let animTimer = null;
const TICK_DELAY = 500;

function selectStrategy(id) {
    // Set the current strategy based on the selected ID (e.g., strategy0, strategy1)
    currentStrategy = `strategy${id}`;

    // Update the strategy label in the UI
    document.getElementById("strategy-label").textContent = `Selected: ${currentStrategy}`;

    // Load all tick data (JSON files) related to this strategy
    loadAllTicks();

    //Show Strategy buttons
    document.getElementById("tick-controls").classList.remove("hidden");
}


async function loadAllTicks() {
    tickData = [];                  // Reset tick data array
    let index = 0;                  // Start from tick_000.json
    let consecutiveFails = 0;       // To count how many files were not found in a row

    while (consecutiveFails < 3) {  // Allow up to 3 missing files before stopping
        const padded = index.toString().padStart(3, '0'); // Format as tick_000, tick_001...
        const url = `../output/simulation/${currentStrategy}/tick_${padded}.json`;

        try {
            const resp = await fetch(url);

            // If the file is missing (e.g. 404), count the failure and skip to next
            if (!resp.ok) {
                consecutiveFails++;
                index++;
                continue;
            }

            const json = await resp.json();  // Parse JSON content
            tickData.push(json);             // Add to array
            index++;
            consecutiveFails = 0;            // Reset fails if fetch was successful

        } catch (err) {
            console.log("Fetch error:", err);  // For network or unexpected errors
            break;
        }
    }

    currentTick = 0;         // Reset to the first tick
    renderTick(0);           // Show first tick on screen

    console.log(`Loaded ${tickData.length} ticks`);
}



async function renderTick(index) {
    // If the index is out of bounds, do nothing
    if (!tickData[index]) return;

    // Load and display the map for the given tick
    await updateMap(index);

    // Update text/labels showing resource stats, villager info, etc.
    updateInfoPanel(index);
}

async function loadTickMap(strategy, tickIndex) {
    // Format tick index as 3 digits (e.g., 001, 002)
    const padded = tickIndex.toString().padStart(3, '0');
    const path = `output/ticks/${strategy}/tick_${padded}.txt`;

    try {
        // Try to fetch the map text file
        const response = await fetch(path);
        if (!response.ok) {
            console.warn("Could not load map txt:", path);
            return null;
        }

        // Read and parse the map into a 2D array of numbers
        const text = await response.text();
        const rows = text.trim().split('\n');
        const map = rows.map(row => {
            return row.trim().split(/\s+/).map(cell => {
                // Mark villagers (V) as 99 for emoji display
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
    const map = await loadTickMap(strategy, tickIndex); // Load map for this tick

    let output = "";

    if (!map) {
        output = "Map not found for tick " + tickIndex;
    } else {
        // Convert the map to emoji characters for visual display
        for (let y = 0; y < map.length; y++) {
            for (let x = 0; x < map[y].length; x++) {
                const val = map[y][x];
                output += CELL_EMOJI[val] || "❓";
            }
            output += "\n";
        }
    }

    // Show the map in the UI
    document.getElementById("map-area").textContent = output;
}



function updateInfoPanel(tickIndex) {
    const tick = tickData[tickIndex];
    if (!tick) return;

    // Update tick number
    document.getElementById("tick-counter").textContent = `Tick = ${tick.tick}`;

    const rightInfo = document.getElementById("right-info");

    // Display resources and performance stats
    rightInfo.innerHTML = `👨‍🌾 x ${tick.villagers.length} | 🌲 ${tick.resources.wood} 🪙 ${tick.resources.gold} 🍖 ${tick.resources.food}<br>`;
    rightInfo.innerHTML += `Effort: ${tick.statistics.collectionEffort} | Dist: ${tick.statistics.totalDistance}<br>`;

    // Show action of each villager (just first action)
    tick.villagers.forEach(v => {
        if (v.actions.length > 0) {
            rightInfo.innerHTML += `V${v.id}: ${v.actions[0]}<br>`;
        }
    });
}


function startAnim() {
    console.log("startAnim called");

    // Don't start if already running or not enough ticks
    if (animTimer !== null || tickData.length <= 1) {
        console.log("Not enough ticks to animate or animation already running");
        return;
    }

    // Async animation step function
    async function step() {
        if (currentTick >= tickData.length) {
            console.log("Animation finished");
            animTimer = null;
            return;
        }

        console.log(`Rendering tick ${currentTick}`);
        await renderTick(currentTick++); // Wait for rendering to finish
        animTimer = setTimeout(() => step(), TICK_DELAY); // Schedule next tick
    }

    step(); // Start animation
}

function goToNextTick() {
    if (currentStrategy === null || tickData.length === 0) return;

    if (currentTick < tickData.length - 1) {
        currentTick++;
        renderTick(currentTick);
    }
}

function goToPreviousTick() {
    if (currentStrategy === null || tickData.length === 0) return;

    if (currentTick > 0) {
        currentTick--;
        renderTick(currentTick);
    }
}

async function playVillagerPaths() {
    if (!tickData[currentTick]) return;

    const strategy = currentStrategy;
    const tickInfo = tickData[currentTick];
    const steps = 3; // Number of steps to simulate movement

    for (let step = 0; step < steps; step++) {
        const map = await loadTickMap(strategy, currentTick);
        if (!map) {
            document.getElementById("map-area").textContent = "Could not load map.";
            return;
        }

        // Place villagers on the map based on their current path step
        tickInfo.villagers.forEach(villager => {
            const [x, y] = villager.path[step];
            if (map[y] && map[y][x] !== undefined) {
                map[y][x] = 99; // 99 = villager emoji
            }
        });

        // Render map with villagers
        let output = "";
        for (let y = 0; y < map.length; y++) {
            for (let x = 0; x < map[y].length; x++) {
                const val = map[y][x];
                output += CELL_EMOJI[val] || "❓";
            }
            output += "\n";
        }

        document.getElementById("map-area").textContent = output;

        await new Promise(resolve => setTimeout(resolve, 300)); // Wait 300ms
    }
}

