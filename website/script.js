console.log("Running")

const boardDiv = document.getElementById("board");
const playerDiv = document.getElementById("player");

let cells = [];
for (let i = 0; i < 6; i++) {
  for (let j = 0; j < 7; j++) {
    let div = document.createElement("div");
    div.dataset.value = "0";
    board.prepend(div);
    cells.push(div)
  }
}

let player = 0;
function Play(column) {
  console.log("play: ", player, column)
  for (row = 0; row < 6; row++) {
    let cell = cells[row*7 + column];
    if (cell.dataset.value == "0") {
      cell.dataset.value = player + 1;
      player = 1 - player;
      console.log("set:", row);
      break;
    }
  }
  playerDiv.innerText = player + 1;
}

let ended = false
let playing = false;
for (let j = 0; j < 7; j++) {
  let button = document.createElement("button");
  button.addEventListener("click", () => {
    if (ended || playing) return;
    playing = true;
    Play(j);
    PlayMcts();
    playing = false;
  })
  board.prepend(button);
}

const strength = 1000;
async function PlayMcts() {
  if (ended) return;
  playing = true;
  let repr = cells.map(cell => cell.dataset.value).join("");
  let request = `/api/mcts?player=m${strength}:${player+1}&states=012&board=${repr}`;
  console.log(request);
  let response = await fetch(request);
  let json = await response.json();
  console.log(json);
  if (json.error) {
    playerDiv.innerText = json.error;
    ended = true;
  } else {
    Play(json.column);
    if (json.result) {
      playerDiv.innerText = "MCTS Wins"
      ended = true;
    }
  }
  if (ended) {
    document.body.dataset.gameOver = "1"
  }
  playing = false;
}

