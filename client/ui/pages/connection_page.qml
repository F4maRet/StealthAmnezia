// StealthAmnezia: Hybrid + Auto-CPS
Column {
    spacing: 10

    RadioButton {
        id: hybridBtn
        text: qsTr("Hybrid (AmneziaWG + XRay Reality)")
        checked: true
        onCheckedChanged: if (checked) ServersModel.currentProtocol = "hybrid"
    }

    Button {
        text: qsTr("Авто-CPS (подобрать под страну)")
        width: parent.width
        onClicked: {
            var country = ServersModel.currentServer.country || "RU"
            var cps = CpsGenerator.autoForCountry(country)
            console.log("✅ Авто-CPS сгенерирован для " + country + ": " + cps)
            // Можно показать уведомление или сохранить в конфиг
            showToast(qsTr("CPS сгенерирован: ") + cps)
        }
    }
}