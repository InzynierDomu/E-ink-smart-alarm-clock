function doGet(e) {
  var calendars = CalendarApp.getAllCalendars();
  if (calendars == undefined) {
    return ContentService
      .createTextOutput(JSON.stringify({
        success: false,
        error: "no access to calendar hubba"
      }))
      .setMimeType(ContentService.MimeType.JSON);
  }

  var calendars_selected = [];
  for (var ii = 0; ii < calendars.length; ii++) {
    if (calendars[ii].isSelected()) {
      calendars_selected.push(calendars[ii]);
    }
  }

  var today = new Date();
  var events = mergeCalendarEventsForToday(calendars_selected, today);

  var eventsArray = [];
  for (var ii = 0; ii < events.length; ii++) {
    var event = events[ii];
    var myStatus = event.getMyStatus();

    var eventObj = {
      startTime: event.getStartTime().toISOString(),
      endTime: event.getEndTime().toISOString(),
      title: event.getTitle(),
      isAllDayEvent: event.isAllDayEvent(),
      description: event.getDescription() || '',
      location: event.getLocation() || '',
      guestStatus: myStatus.toString(),
      calendarName: event.getOriginalCalendarId(),
      id: event.getId()
    };
    eventsArray.push(eventObj);
  }

  var response = {
    success: true,
    date: today.toISOString(),
    eventsCount: eventsArray.length,
    events: eventsArray
  };

  return ContentService
    .createTextOutput(JSON.stringify(response))
    .setMimeType(ContentService.MimeType.JSON);
}

function mergeCalendarEventsForToday(calendars, dayDate) {
  var params = { day: dayDate, uniqueIds: [] };
  return calendars.map(getUniqueEventsForDay_, params)
                  .reduce(toSingleArray_)
                  .sort(byStart_);
}

function getUniqueEventsForDay_(calendar) {
  return calendar.getEventsForDay(this.day)
                 .filter(onlyUniqueEvents_, this.uniqueIds);
}

function onlyUniqueEvents_(event) {
  var eventId = event.getId();
  var uniqueEvent = this.indexOf(eventId) < 0;
  if (uniqueEvent) this.push(eventId);
  return uniqueEvent;
}

function toSingleArray_(a, b) { return a.concat(b); }

function byStart_(a, b) {
  return a.getStartTime().getTime() - b.getStartTime().getTime();
}
