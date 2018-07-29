function secondsToMMSS(seconds)
{
    var remainingSec = Math.floor(seconds)
    var durationMin = Math.floor(remainingSec / 60)
    var durationSec = remainingSec % 60

    return durationMin + ":" + (durationSec < 10 ? "0" + durationSec : durationSec)
}

function panelSize(x, y)
{
    return x + "x" + y
}
