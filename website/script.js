console.log("Running")

const board = document.getElementById("board");

for (let i = 0; i < 7; i++) {
  for (let j = 0; j < 6; j++) {
    let div = document.createElement("div");
    div.dataset.value = "-";
    div.id = `cell-${i}:${j}`
    board.append(div)
  }
}